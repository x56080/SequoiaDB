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

package com.sequoiadb.flink.table;

import com.sequoiadb.flink.codec.SDBDataConverter;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.sink.SDBSink;

import org.apache.flink.table.connector.ChangelogMode;
import org.apache.flink.table.connector.sink.DynamicTableSink;
import org.apache.flink.table.connector.sink.SinkProvider;
import org.apache.flink.table.data.RowData;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.logical.RowType;
import org.apache.flink.types.RowKind;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SDBDynamicTableSink implements DynamicTableSink {

    private SDBSinkOptions sdbOptions;
    private DataType producedDataType;
    private int parallel;

    private final static Logger LOG = LoggerFactory.getLogger(SDBDynamicTableSink.class);

    /*
     * Constructor of SDBDynamicTableSink
     * @param sdboptions            passdown options from user defined options
     * @param producedDataType      table schema info
     */
    public SDBDynamicTableSink(
        SDBSinkOptions sdboptions,
        DataType producedDataType) {
            this.sdbOptions = sdboptions;
            this.producedDataType = producedDataType;
            this.parallel = sdboptions.getSinkParallelism();
    }

    /*
     * returns current sink supported changelogmodes
     * @param requestedMode         requested Changelogmode
     * @return                      supported changelog mode
     */
    @Override
    public ChangelogMode getChangelogMode(ChangelogMode requestedMode) {
        ChangelogMode changeLogMode = ChangelogMode
            .newBuilder().addContainedKind(RowKind.INSERT).build();
        return changeLogMode;
    }

    /*
     * returns a sinkRuntimeProvider that carrys a SDBSink
     * @param context               carry runtime info
     * @return                      SinkProvider of SDBSink
     * @see                         Context
     */
    @Override
    public SinkRuntimeProvider getSinkRuntimeProvider(Context context) {
        LOG.debug("create sink runtime provider");
        SDBDataConverter dataConverter
            = new SDBDataConverter(((RowType) producedDataType.getLogicalType()));
        return SinkProvider.of( new SDBSink<RowData>(sdbOptions, dataConverter), parallel);
    }

    @Override
    public DynamicTableSink copy() {
        return new SDBDynamicTableSink(sdbOptions, producedDataType);
    }

    @Override
    public String asSummaryString() {
        return "SDB Dynamic Table Sink";
    }

}