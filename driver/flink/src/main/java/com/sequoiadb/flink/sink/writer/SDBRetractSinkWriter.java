/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.flink.sink.writer;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.common.client.SDBClientProvider;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.util.RetryUtil;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.sink.state.KafkaKey;

import org.apache.commons.compress.utils.Lists;
import org.apache.flink.api.connector.sink.SinkWriter;
import org.apache.flink.table.api.DataTypes;
import org.apache.flink.table.data.RowData;
import org.apache.flink.table.data.TimestampData;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.logical.RowType;
import org.apache.flink.types.RowKind;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.*;
import java.util.stream.Stream;

import static com.sequoiadb.flink.sink.writer.SDBRetractSinkWriter.SupportedMetadata.*;

public class SDBRetractSinkWriter implements SinkWriter<RowData, Void, Void> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBRetractSinkWriter.class);

    private static final Set<SupportedMetadata> REQUIRED_METADATA = new HashSet<>();

    static {
        REQUIRED_METADATA.add(TOPIC);
        REQUIRED_METADATA.add(PARTITION);
        REQUIRED_METADATA.add(OFFSET);
        REQUIRED_METADATA.add(EVENT_TIMESTAMP);
        REQUIRED_METADATA.add(EXTRA_OP_TYPE);
        REQUIRED_METADATA.add(EXTRA_PROMISE);
    }

    private static final int DEFAULT_RETRY_TIMES = 3;
    private static final int DEFAULT_RETRY_INTERVAL = 100;

    // EXTRA OP TYPE
    private static final String UPDATE_PK_BEFORE = "UPDATE_PK_BEFORE";
    private static final String UPDATE_PK_AFTER = "UPDATE_PK_AFTER";

    // BSON Modifier Type
    private static final String MODIFIER_SET = "$set";

    // promise collection suffix
    private static final String PROMISE_TEMP_CL_SUFFIX = "_temp_promise_cl";
    private static final String PROMISE = "promise";

    // primary key for upsert/delete
    private final String[] upsertKeys;
    private final int[] metadataPositions;

    private final SDBDataConverter converter;
    private final SDBSinkOptions sinkOptions;

    // group changelogs by KafkaKey(topic, partition)
    private final Map<KafkaKey, PriorityQueue<RowData>> bypassLogsMap = new HashMap<>();
    private final Map<KafkaKey, TimestampData> latestCommitTsMap = new HashMap<>();

    private transient final SDBClientProvider provider;

    // promise temp cl's name
    private final String promiseCl;

    private boolean hasMultiPartition;
    private final List<String> promises = new ArrayList<>();

    public SDBRetractSinkWriter(
            SDBDataConverter converter, SDBSinkOptions sinkOptions) {
        this.upsertKeys = sinkOptions.getUpsertKey();
        RowType rowType = converter.getRowType();
        this.metadataPositions = getMetadataPositions(rowType);

        this.converter = converter;
        this.sinkOptions = sinkOptions;

        this.provider = SDBClientProvider.builder()
                .withHosts(sinkOptions.getHosts())
                .withCollectionSpace(sinkOptions.getCollectionSpace())
                .withCollection(sinkOptions.getCollection())
                .withUsername(sinkOptions.getUsername())
                .withPassword(sinkOptions.getPassword())
                .withOptions(sinkOptions)
                .build();
        this.promiseCl = sinkOptions.getCollection() + PROMISE_TEMP_CL_SUFFIX;
        ensurePromiseCl(promiseCl);
        this.hasMultiPartition = sinkOptions.hasMultiPartition();
    }

    private void ensurePromiseCl(String promiseClName) {
        try {
            provider.getCollectionSpace()
                    .createCollection(promiseClName);
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_DMS_EXIST.getErrorCode()) {
                // ignored if collection is already exist.
            } else {
                throw ex;
            }
        }
    }

    /**
     *  1. if upstream kafka only has one topic with one partition, the changelog stream
     *     collected by sink is always ordered, it can flush directly.
     *  2. otherwise, changelogs stream needs to be partitioned by KafkaKey(topic, partition)
     *     , so that each partition of changelogs stream is ordered again.
     */
    @Override
    public void write(RowData element, Context context)
            throws IOException, InterruptedException {
        if (!hasMultiPartition) {
            flushRow(element);
        } else {
            // read metadata from physical row
            String extraOp = readMetadata(element, EXTRA_OP_TYPE);
            KafkaKey key = retrieveKeyFromRow(element);
            TimestampData currEventTs = readMetadata(element, EVENT_TIMESTAMP);

            if (UPDATE_PK_AFTER.equals(extraOp)) {
                // means this changelog might come from other streaming, needs to add to bypass queue.
                PriorityQueue<RowData> bpLogs = bypassLogsMap.get(key);
                if (bpLogs == null) {
                    bpLogs = new PriorityQueue<>(new RowTsComparator());
                    bypassLogsMap.put(key, bpLogs);
                }
                bpLogs.offer(element);
            } else {
                // flush bypass changelogs before current changelog.
                // consider as merging two ordered streaming.
                flushByPass(key, currEventTs);
                flushRow(element);
                updateCommitTs(key, currEventTs); // update the latest commit ts, for skip late by pass changelog.
            }
        }
    }

    class RowTsComparator implements Comparator<RowData> {
        @Override
        public int compare(RowData o1, RowData o2) {
            TimestampData t1 = readMetadata(o1, EVENT_TIMESTAMP);
            TimestampData t2 = readMetadata(o2, EVENT_TIMESTAMP);
            if (t1 == null || t2 == null) {
                throw new SDBException("can not retrieve event timestamp from changelog.");
            }
            return t1.compareTo(t2);
        }
    }

    /**
     * flush row data (changelog) by RowKind.
     *  1. for INSERT, UPDATE_AFTER, DELETE, we can upsert/delete directly.
     *  2. for UPDATE_PK_BEFORE, we will insert a promise to temporal table, to tell all sink
     *      the changelog has already been processed.
     *  3. for UPDATE_PK_AFTER, we have to wait util UPDATE_PK_BEFORE has already been processed.
     *      We are using temporal table to sync these two operation.
     *
     * @param rowData
     */
    private void flushRow(RowData rowData) {
        BSONObject record = converter
                .toExternal(rowData, sinkOptions.getIgnoreNullField());

        switch (rowData.getRowKind()) {
            case INSERT:
            case UPDATE_AFTER:
                String extraOp =
                        readMetadata(rowData, SupportedMetadata.EXTRA_OP_TYPE);
                if (UPDATE_PK_AFTER.equals(extraOp)) {
                    flushOnSync(rowData);
                } else {
                    provider.getCollection().upsert(
                            createMatcher(record),
                            createModifier(MODIFIER_SET, record),
                            null,
                            record, 0);
                }
                break;

            case UPDATE_BEFORE:
            case DELETE:
                provider.getCollection().delete(createMatcher(record));
                if (rowData.getRowKind() == RowKind.UPDATE_BEFORE) {
                    String uniquePromise =
                            readMetadata(rowData, SupportedMetadata.EXTRA_PROMISE);
                    // add promise to local
                    promises.add(uniquePromise);
                    // add promise to temp table
                    BSONObject promise = createPromise(uniquePromise);
                    provider.getCollectionSpace().getCollection(promiseCl)
                            .upsert(promise,
                                    createModifier(MODIFIER_SET, promise),
                                    null, promise);
                }
                break;
        }
    }

    /**
     * flush changelog (RowKind is UPDATE_PK_AFTER),
     * it should wait util the corresponding UPDATE_PK_BEFORE is done.
     *
     * Notes:
     *  UPDATE_PK_BEFORE, UPDATE_PK_AFTER will hold the same promise(UUID)
     *  for synchronization.
     *
     * @param rowData
     */
    private void flushOnSync(RowData rowData) {
        BSONObject record = converter
                .toExternal(rowData, sinkOptions.getIgnoreNullField());
        String uniquePromise = readMetadata(rowData, SupportedMetadata.EXTRA_PROMISE);

        Optional<String> result = RetryUtil.retry(
                this::checkIfPromiseExists,
                () -> retrievePromise(uniquePromise),
                DEFAULT_RETRY_TIMES,
                DEFAULT_RETRY_INTERVAL);

        if (!result.isPresent()) {
            provider.getCollection().upsert(
                    createMatcher(record),
                    createModifier(MODIFIER_SET, record),
                    null,
                    record, 0);
        } else {
            // log topic, partition, offset for data repair,
            // user can decide whether to write the failed data.
            LOG.warn("cannot flush changelog to SequoiaDB, topic: {}, partition: {}, offset: {}, op type: {}, " +
                            "target collection: {}.",
                    readMetadata(rowData, TOPIC),
                    readMetadata(rowData, PARTITION),
                    readMetadata(rowData, OFFSET),
                    UPDATE_PK_AFTER,
                    String.join(".", sinkOptions.getCollectionSpace(), sinkOptions.getCollection()));
        }
        cleanUpPromise(uniquePromise);
    }

    private void cleanUpPromise(String uniquePromise) {
        promises.remove(uniquePromise);
        provider.getCollectionSpace()
                .getCollection(promiseCl).delete(createPromise(uniquePromise));
    }

    private void flushByPass(
            KafkaKey key, TimestampData currEventTs) {
        Queue<RowData> bpLogs = bypassLogsMap.get(key);
        if (bpLogs == null) {
            return;
        }

        while (!bpLogs.isEmpty()) {
            RowData rowData = bpLogs.peek();
            TimestampData lCommitTs = latestCommitTsMap.get(key);
            TimestampData rEventTs = readMetadata(rowData, EVENT_TIMESTAMP);
            if (lCommitTs == null || currEventTs == null) {
                break;
            }

            assert rEventTs != null;
            // not in current window
            if (rEventTs.compareTo(lCommitTs) < 0) {
                bpLogs.poll();
                LOG.warn("cannot flush changelog to SequoiaDB, topic: {}, partition: {}, offset: {}, op type: {}, " +
                                "target collection: {}.",
                        readMetadata(rowData, TOPIC),
                        readMetadata(rowData, PARTITION),
                        readMetadata(rowData, OFFSET),
                        UPDATE_PK_AFTER,
                        String.join(".", sinkOptions.getCollectionSpace(), sinkOptions.getCollection()));
                continue;
            }

            if (rEventTs.compareTo(currEventTs) > 0) {
                break;
            }
            flushOnSync(rowData);
        }
    }

    // compute metadata position (in RowType)
    private int[] getMetadataPositions(RowType rowType) {
        return Stream.of(SupportedMetadata.values())
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

    // read metadata in physical row data (changelog)
    private <T> T readMetadata(RowData rowData, SupportedMetadata metadata) {
        final int pos = metadataPositions[metadata.ordinal()];
        if (pos < 0) {
            // single partition only take care about 'EXTRA_PROMISE'
            if (!hasMultiPartition &&
                (!metadata.equals(EXTRA_PROMISE) && !metadata.equals(EXTRA_OP_TYPE))) {
                return null;
            }
            throw new SDBException(
                    String.format("can't perform retract write without defining column: %s", metadata.key));
        }
        return (T) metadata.converter.convert(rowData, pos);
    }

    private KafkaKey retrieveKeyFromRow(RowData rowData) {
        return new KafkaKey(
                readMetadata(rowData, SupportedMetadata.TOPIC),
                readMetadata(rowData, SupportedMetadata.PARTITION));
    }

    @Override
    public List<Void> prepareCommit(boolean flush) {
        return Lists.newArrayList();
    }

    @Override
    public List<Void> snapshotState(long checkpointId) throws IOException {
        return Lists.newArrayList();
    }

    // update the latest commit ts for specified kafka partition
    private void updateCommitTs(KafkaKey key, TimestampData eventTs) {
        latestCommitTsMap.put(key, eventTs);
    }

    // retrieve unique promise from local cache or temp table
    private String retrievePromise(String uniquePromise) {
        if (promises.contains(uniquePromise)) {
            return uniquePromise;
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put(PROMISE, uniquePromise);
        DBCollection temp = provider.getCollectionSpace()
                .getCollection(promiseCl);

        DBCursor cursor = temp.query(matcher, null, null, null);
        if (cursor.hasNext()) {
            return (String) cursor.getNext().get(PROMISE);
        }

        return null;
    }

    private boolean checkIfPromiseExists(String uniquePromise) {
        return uniquePromise != null;
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

    private BSONObject createPromise(String uniquePromise) {
        BSONObject promise = new BasicBSONObject();
        promise.put(PROMISE, uniquePromise);
        return promise;
    }

    @Override
    public void close() throws Exception {
        if (provider != null) {
            provider.close();
        }
    }

    enum SupportedMetadata {
        // Kafka Metadata
        TOPIC(
                "$kafka-topic",
                DataTypes.STRING().nullable(),
                new MetadataConverter() {
                    @Override
                    public Object convert(RowData consumedRow, int pos) {
                        return consumedRow.getString(pos).toString();
                    }
                }),

        PARTITION(
                "$kafka-partition",
                DataTypes.INT().nullable(),
                new MetadataConverter() {
                    @Override
                    public Object convert(RowData consumedRow, int pos) {
                        return consumedRow.getInt(pos);
                    }
                }),

        OFFSET(
                "$kafka-offset",
                DataTypes.INT().nullable(),
                new MetadataConverter() {
                    @Override
                    public Object convert(RowData consumedRow, int pos) {
                        return consumedRow.getInt(pos);
                    }
                }),

        EVENT_TIMESTAMP(
                "$event-timestamp",
                DataTypes.TIMESTAMP(3).nullable(),
                new MetadataConverter() {
                    @Override
                    public Object convert(RowData consumedRow, int pos) {
                        return consumedRow.getTimestamp(pos, 3);
                    }
                }),

        // EXTRA TOKEN
        EXTRA_OP_TYPE(
                "$extra-op-type",
                DataTypes.STRING().nullable(),
                new MetadataConverter() {
                    @Override
                    public Object convert(RowData consumedRow, int pos) {
                        return consumedRow.getString(pos).toString();
                    }
                }),

        EXTRA_PROMISE(
                "$extra-promise",
                DataTypes.STRING().nullable(),
                new MetadataConverter() {
                    @Override
                    public Object convert(RowData consumedRow, int pos) {
                        if (consumedRow.isNullAt(pos)) {
                            return null;
                        }
                        return consumedRow.getString(pos).toString();
                    }
                }),
        ;

        final String key;

        final DataType dataType;

        final MetadataConverter converter;

        SupportedMetadata(String key, DataType dataType, MetadataConverter converter) {
            this.key = key;
            this.dataType = dataType;
            this.converter = converter;
        }
    }

    interface MetadataConverter {
        Object convert(RowData consumedRow, int pos);
    }

}

