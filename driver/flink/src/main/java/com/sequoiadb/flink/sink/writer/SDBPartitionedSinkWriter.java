package com.sequoiadb.flink.sink.writer;

import com.sequoiadb.base.result.DeleteResult;
import com.sequoiadb.base.result.UpdateResult;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.common.client.SDBClientProvider;
import com.sequoiadb.flink.common.metadata.ExtraRowKind;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.sink.state.EventState;
import org.apache.commons.compress.utils.Lists;
import org.apache.flink.api.connector.sink.SinkWriter;
import org.apache.flink.api.java.tuple.Tuple3;
import org.apache.flink.table.api.DataTypes;
import org.apache.flink.table.data.RowData;
import org.apache.flink.table.data.TimestampData;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.logical.RowType;
import org.apache.flink.types.RowKind;
import org.apache.flink.util.concurrent.ExecutorThreadFactory;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.time.Duration;
import java.time.LocalDateTime;
import java.util.*;
import java.util.concurrent.*;
import java.util.stream.Stream;

import static com.sequoiadb.flink.sink.writer.SDBPartitionedSinkWriter.ReadableMetadata.EXTRA_ROW_KIND;

public class SDBPartitionedSinkWriter
        implements SinkWriter<RowData, Void, Map<BSONObject, EventState>> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBPartitionedSinkWriter.class);

    private static final String MODIFIER_SET = "$set";

    private final String[] upsertKeys;
    private final int[] metadataPositions;

    private final String eventTsFieldName;
    private final int eventTsPos;

    private Map<BSONObject, EventState> stateMap = new ConcurrentHashMap<>();
    private final Map<BSONObject,
            Queue<Tuple3<RowKind, BSONObject, TimestampData>>> blockedMap = new HashMap<>();

    private final SDBDataConverter converter;
    private final SDBSinkOptions sinkOptions;

    private transient final SDBClientProvider provider;

    private transient ScheduledExecutorService stateCleanupDaemon;
    private transient ScheduledFuture cleanupFuture;

    public SDBPartitionedSinkWriter(
            SDBDataConverter converter, SDBSinkOptions sinkOptions, List<Map<BSONObject, EventState>> states) {
        this.upsertKeys = sinkOptions.getUpsertKey();
        this.eventTsFieldName = sinkOptions.getEventTsFieldName();

        RowType rowType = converter.getRowType();
        this.metadataPositions = getMetadataPositions(rowType);
        this.eventTsPos = getEventTsPos(rowType);

        this.provider = SDBClientProvider.builder()
                .withHosts(sinkOptions.getHosts())
                .withCollectionSpace(sinkOptions.getCollectionSpace())
                .withCollection(sinkOptions.getCollection())
                .withUsername(sinkOptions.getUsername())
                .withPassword(sinkOptions.getPassword())
                .withOptions(sinkOptions)
                .build();
        this.converter = converter;
        this.sinkOptions = sinkOptions;

        if (!states.isEmpty()) {
            this.stateMap = new ConcurrentHashMap<>(states.get(0));
            this.stateMap.forEach((pk, state) -> {
                state.setProcessingTime(TimestampData.fromLocalDateTime(LocalDateTime.now()));
            });
        }

        if (sinkOptions.getStateTtl() > 0) {
            final int stateTtl = sinkOptions.getStateTtl();
            this.stateCleanupDaemon =
                    Executors.newScheduledThreadPool(
                            1, new ExecutorThreadFactory("sequoiadb-retract-state-cleaner"));
            this.cleanupFuture =
                    stateCleanupDaemon.scheduleAtFixedRate(
                            () -> {
                                Iterator<Map.Entry<BSONObject, EventState>> iterator = stateMap
                                        .entrySet()
                                        .iterator();
                                while (iterator.hasNext()) {
                                    Map.Entry<BSONObject, EventState> entry = iterator.next();

                                    LocalDateTime processingTime = entry.getValue()
                                            .getProcessingTime()
                                            .toLocalDateTime();
                                    Duration duration = Duration.between(processingTime, LocalDateTime.now());
                                    if (duration.toMinutes() > stateTtl) {
                                        iterator.remove();
                                    }
                                }
                            },
                            stateTtl,
                            stateTtl,
                            TimeUnit.MINUTES);
        }
    }

    @Override
    public void write(RowData changelog, Context context) throws IOException, InterruptedException {
        ExtraRowKind rowKind = ExtraRowKind.from(
                Objects.requireNonNull(readMetadata(changelog, EXTRA_ROW_KIND)));
        switch (rowKind) {
            case INSERT:
                handleInsert(changelog);
                break;
            case UPDATE_AFT:
                handleUpdate(changelog);
                break;
            case DELETE:
                handleDelete(changelog);
                break;
            case UPDATE_PK_BEF:
            case UPDATE_PK_AFT:
                handlePkUpdate(rowKind, changelog);
                break;
        }
    }

    private boolean checkIfOutOfDate(
            BSONObject matcher, TimestampData currentEventTs) {
        EventState state = stateMap.get(matcher);

        if (state != null) {
            TimestampData latestUpdatePkTs = state.getEventTime();
            if (currentEventTs.compareTo(latestUpdatePkTs) <= 0) {
                return true;
            }
        }
        return false;
    }

    // ============ Handle Func ================

    private void handleInsert(RowData changelog) {
        BSONObject record = converter
                .toExternal(changelog, sinkOptions.getIgnoreNullField());
        BSONObject matcher = createMatcher(record);
        TimestampData currEventTs = readEventTs(changelog);

        if (checkIfOutOfDate(matcher, currEventTs)) {
            return;
        }

        if (blockedMap.containsKey(matcher)) {
            blockedMap.get(matcher)
                      .offer(new Tuple3<>(RowKind.INSERT, record, currEventTs));
            return;
        }

        try {
            provider.getCollection()
                    .insertRecord(record);
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_IXM_DUP_KEY.getErrorCode()) {
                Queue<Tuple3<RowKind, BSONObject, TimestampData>> blockedChangelogs = blockedMap.get(matcher);
                if (blockedChangelogs == null) {
                    blockedChangelogs = new ArrayDeque<>();
                    blockedMap.put(matcher, blockedChangelogs);
                }
                blockedChangelogs.offer(new Tuple3<>(RowKind.INSERT, record, currEventTs));
            } else {
                throw ex;
            }
        }
    }

    private void handleUpdate(RowData changelog) {
        BSONObject record = converter
                .toExternal(changelog, sinkOptions.getIgnoreNullField());
        BSONObject matcher = createMatcher(record);
        TimestampData currEventTs = readEventTs(changelog);

        if (checkIfOutOfDate(matcher, currEventTs)) {
            return;
        }

        if (blockedMap.containsKey(matcher)) {
            blockedMap.get(matcher)
                      .offer(new Tuple3<>(RowKind.UPDATE_AFTER, record, currEventTs));
            return;
        }

        UpdateResult result = provider
                .getCollection()
                .updateRecords(matcher, createModifier(MODIFIER_SET, record));
        if (0 == result.getModifiedNum()) {
            Queue<Tuple3<RowKind, BSONObject, TimestampData>> blockedChangelogs = blockedMap.get(matcher);
            if (blockedChangelogs == null) {
                blockedChangelogs = new ArrayDeque<>();
                blockedMap.put(matcher, blockedChangelogs);
            }
            blockedChangelogs.offer(new Tuple3<>(RowKind.UPDATE_AFTER, record, currEventTs));
        }
    }

    private void handleDelete(RowData changelog) {
        BSONObject record = converter
                .toExternal(changelog, sinkOptions.getIgnoreNullField());
        BSONObject matcher = createMatcher(record);
        TimestampData currEventTs = readEventTs(changelog);

        if (checkIfOutOfDate(matcher, currEventTs)) {
            return;
        }

        if (blockedMap.containsKey(matcher)) {
            blockedMap.get(matcher)
                      .offer(new Tuple3<>(RowKind.DELETE, record, currEventTs));
        }

        DeleteResult result = provider
                .getCollection()
                .deleteRecords(matcher);
        if (0 == result.getDeletedNum()) {
            Queue<Tuple3<RowKind, BSONObject, TimestampData>> blockedChangelogs = blockedMap.get(matcher);
            if (blockedChangelogs == null) {
                blockedChangelogs = new ArrayDeque<>();
                blockedMap.put(matcher, blockedChangelogs);
            }
            blockedChangelogs.offer(new Tuple3<>(RowKind.DELETE, record, currEventTs));
        }
    }

    // ================ pk update handler ===============
    private void handlePkUpdate(ExtraRowKind rowKind, RowData changelog) {
        BSONObject record = converter
                .toExternal(changelog, sinkOptions.getIgnoreNullField());
        BSONObject matcher = createMatcher(record);
        TimestampData currEventTs = readEventTs(changelog);

        if (stateMap.containsKey(matcher)) {
            TimestampData latestUpdatePkTs = stateMap.get(matcher)
                    .getEventTime();
            if (currEventTs.compareTo(latestUpdatePkTs) < 0) {
                return;
            }
        }

        // update event timestamp
        EventState state = new EventState(currEventTs,
                TimestampData.fromLocalDateTime(LocalDateTime.now()));
        stateMap.put(matcher, state);

        if (rowKind.equals(ExtraRowKind.UPDATE_PK_AFT)) {
            provider.getCollection()
                    .upsertRecords(matcher, createModifier(MODIFIER_SET, record));
        } else if (rowKind.equals(ExtraRowKind.UPDATE_PK_BEF)) {
            provider.getCollection()
                    .deleteRecords(matcher);
        }

        // flush blocked queue
        flushBlockedQueue(matcher);
    }

    private void flushBlockedQueue(BSONObject matcher) {
        Queue<Tuple3<RowKind, BSONObject, TimestampData>> blockedQueue = blockedMap.get(matcher);
        if (blockedQueue == null) {
            return;
        }

        EventState state = stateMap.get(matcher);
        if (state != null) {
            TimestampData latestUpdatePkTs = state.getEventTime();
            while (!blockedQueue.isEmpty() &&
                    blockedQueue.peek().f2.compareTo(latestUpdatePkTs) < 0) {
                // skip expired records
                blockedQueue.poll();
            }
        }

        while (!blockedQueue.isEmpty()) {
            Tuple3<RowKind, BSONObject, TimestampData> tuple = blockedQueue.poll();
            BSONObject record = tuple.f1;

            // tuple.f0 -> row kind (op type) of this record
            switch (tuple.f0) {
                case INSERT:
                    provider.getCollection()
                            .insertRecord(record);
                    break;

                case UPDATE_AFTER:
                    provider.getCollection()
                            .updateRecords(matcher, createModifier(MODIFIER_SET, record));
                    break;

                case UPDATE_BEFORE:
                case DELETE:
                    provider.getCollection()
                            .deleteRecords(matcher);
                    break;
            }
        }
    }

    private BSONObject createMatcher(BSONObject record) {
        BSONObject matcher = new BasicBSONObject();

        for (String upsertKey : upsertKeys) {
            matcher.put(upsertKey, record.get(upsertKey));
        }
        return matcher;
    }

    private BSONObject createModifier(String mType, BSONObject updater) {
        BSONObject modifier = new BasicBSONObject();
        modifier.put(mType, updater);
        return modifier;
    }

    @Override
    public List<Void> prepareCommit(boolean flush) throws IOException, InterruptedException {
        return Lists.newArrayList();
    }

    // checkpoint
    @Override
    public List<Map<BSONObject, EventState>> snapshotState(long checkpointId)
            throws IOException {
        List<Map<BSONObject, EventState>> stateList = new ArrayList<>();
        stateList.add(stateMap);
        return stateList;
    }

    @Override
    public void close() throws Exception {
        if (provider != null) {
            provider.close();
        }

        if (stateCleanupDaemon != null) {
            stateCleanupDaemon.shutdown();
        }
    }

    enum ReadableMetadata {
        EXTRA_ROW_KIND(
                "$extra-row-kind",
                DataTypes.INT().nullable(),
                new MetadataConverter() {
                    @Override
                    public Object convert(RowData consumedRow, int pos) {
                        if (pos < 0 || consumedRow.isNullAt(pos)) {
                            return null;
                        }
                        return consumedRow.getInt(pos);
                    }
                }),
        ;

        final String key;
        final DataType dataType;
        final MetadataConverter converter;

        ReadableMetadata(String key,
                         DataType dataType,
                         MetadataConverter converter) {
            this.key = key;
            this.dataType = dataType;
            this.converter = converter;
        }
    }

    interface MetadataConverter {
        Object convert(RowData consumedRow, int pos);
    }

    private int[] getMetadataPositions(RowType rowType) {
        return Stream.of(ReadableMetadata.values())
                .mapToInt(
                        m -> {
                            final int pos = rowType.getFieldNames().indexOf(m.key);
                            if (pos < 0) {
                                return -1;
                            }
                            return pos;
                        })
                .toArray();
    }

    private <T> T readMetadata(RowData rowData, ReadableMetadata metadata) {
        final int pos = metadataPositions[metadata.ordinal()];
        if (pos < 0) {
            return null;
        }
        return (T) metadata.converter.convert(rowData, pos);
    }

    private int getEventTsPos(RowType rowType) {
        int pos = rowType
                .getFieldNames().indexOf(eventTsFieldName);
        if (pos < 0) {
            return -1;
        }
        return pos;
    }

    private TimestampData readEventTs(RowData changelog) {
        return changelog.getTimestamp(eventTsPos, 9);
    }

}
