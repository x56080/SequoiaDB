package com.sequoiadb.flink.sink;

import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.sink.state.EventState;
import com.sequoiadb.flink.sink.state.EventStateSerializer;
import com.sequoiadb.flink.sink.writer.SDBPartitionedSinkWriter;
import org.apache.flink.api.connector.sink.Committer;
import org.apache.flink.api.connector.sink.GlobalCommitter;
import org.apache.flink.api.connector.sink.Sink;
import org.apache.flink.api.connector.sink.SinkWriter;
import org.apache.flink.core.io.SimpleVersionedSerializer;
import org.apache.flink.table.data.RowData;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public class SDBPartitionedSink
        implements Sink<RowData, Void, Map<BSONObject, EventState>, Void> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBPartitionedSink.class);

    private SDBDataConverter converter;

    private SDBSinkOptions sinkOptions;

    public SDBPartitionedSink(SDBDataConverter converter, SDBSinkOptions sinkOptions) {
        this.converter = converter;
        this.sinkOptions = sinkOptions;
    }

    @Override
    public SinkWriter<RowData, Void, Map<BSONObject, EventState>> createWriter(
            InitContext context,
            List<Map<BSONObject, EventState>> states) throws IOException {
        return new SDBPartitionedSinkWriter(converter, sinkOptions, states);
    }

    @Override
    public Optional<SimpleVersionedSerializer<Map<BSONObject, EventState>>> getWriterStateSerializer() {
        return Optional.of(new EventStateSerializer());
    }

    // =============================================================
    // Do not implement these methods which are for 2-PC Committer
    // =============================================================

    @Override
    public Optional<Committer<Void>> createCommitter() throws IOException {
        return Optional.empty();
    }

    @Override
    public Optional<GlobalCommitter<Void, Void>> createGlobalCommitter() throws IOException {
        return Optional.empty();
    }

    @Override
    public Optional<SimpleVersionedSerializer<Void>> getCommittableSerializer() {
        return Optional.empty();
    }

    @Override
    public Optional<SimpleVersionedSerializer<Void>> getGlobalCommittableSerializer() {
        return Optional.empty();
    }
}
