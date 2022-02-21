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

import java.util.HashSet;
import java.util.Set;

import com.sequoiadb.flink.config.SDBOptions;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.config.SDBSourceOptions;

import org.apache.flink.configuration.ConfigOption;
import org.apache.flink.configuration.ReadableConfig;
import org.apache.flink.table.connector.sink.DynamicTableSink;
import org.apache.flink.table.connector.source.DynamicTableSource;
import org.apache.flink.table.factories.DynamicTableSinkFactory;
import org.apache.flink.table.factories.DynamicTableSourceFactory;
import org.apache.flink.table.factories.FactoryUtil;
import org.apache.flink.table.types.DataType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SDBDynamicTableFactory implements DynamicTableSourceFactory, DynamicTableSinkFactory {

    public static final String IDENTIFIER = "sequoiadb";
    
    private final static Logger LOG = LoggerFactory.getLogger(SDBDynamicTableFactory.class);

    // This is how Flink id our factory
    @Override
    public String factoryIdentifier() {
        return IDENTIFIER;
    }

    // required options from WITH
    @Override
    public Set<ConfigOption<?>> requiredOptions() {
        final Set<ConfigOption<?>> options = new HashSet<>();
        options.add(SDBOptions.HOSTS);
        options.add(SDBOptions.COLLECTION_SPACE);
        options.add(SDBOptions.COLLECTION);
        return options;
    }

    // optialnal options form WITH
    @Override
    public Set<ConfigOption<?>> optionalOptions() {
        
        final Set<ConfigOption<?>> optionalOptions = new HashSet<>();
        optionalOptions.add(SDBOptions.USERNAME);
        optionalOptions.add(SDBOptions.PASSWORD_TYPE);
        optionalOptions.add(SDBOptions.PASSWORD);
        optionalOptions.add(SDBOptions.TOKEN);
        optionalOptions.add(SDBOptions.BULK_SIZE);
        optionalOptions.add(SDBOptions.SPLIT_MODE);
        optionalOptions.add(SDBOptions.SPLIT_BLOCK_NUM);
        optionalOptions.add(SDBOptions.PREFERRED_INSTANCE);
        optionalOptions.add(SDBOptions.PREFERRED_INSTANCE_MODE);
        optionalOptions.add(SDBOptions.PREFERRED_INSTANCE_STRICT);
        optionalOptions.add(SDBOptions.IGNORE_NULL_FIELD);
        optionalOptions.add(SDBOptions.PAGE_SIZE);
        optionalOptions.add(SDBOptions.DOMAIN);
        optionalOptions.add(SDBOptions.SHARDING_KEY);
        optionalOptions.add(SDBOptions.SHARDING_TYPE);
        optionalOptions.add(SDBOptions.REPL_SIZE);
        optionalOptions.add(SDBOptions.COMPRESSION_TYPE);
        optionalOptions.add(SDBOptions.AUTO_SPLIT);
        optionalOptions.add(SDBOptions.GROUP);
        optionalOptions.add(SDBOptions.FORMAT);
        optionalOptions.add(SDBOptions.SINK_PARALLELISM);
        optionalOptions.add(SDBOptions.TRANSACTION_ON);
        optionalOptions.add(SDBOptions.MAX_BULK_FILL_TIME);
        return optionalOptions;
    }
    // create SDB DynamicTable sink
    @Override
    public DynamicTableSink createDynamicTableSink(Context context) {
        LOG.debug("create dynamic table sink");

        // use FactoryUtil Helper to get and validate options.
        final FactoryUtil.TableFactoryHelper helper = FactoryUtil.createTableFactoryHelper(this, context);

        helper.validate();

        final ReadableConfig options = helper.getOptions();

        // create sink options, so we dont need to carry everything down.
        SDBSinkOptions sdboptions = new SDBSinkOptions(options);

        // get Datatype from schema, this will be carry to format serializer.
        final DataType producedDataType =
            context.getCatalogTable().getResolvedSchema().toPhysicalRowDataType();

        return new SDBDynamicTableSink(sdboptions, producedDataType);
    }

    //create DynamicTable source
    @Override
    public DynamicTableSource createDynamicTableSource(Context context) {
        FactoryUtil.TableFactoryHelper helper = FactoryUtil.createTableFactoryHelper(this, context);
        helper.validate();

        DataType produceDatatype = context.getCatalogTable()
                .getResolvedSchema()
                .toPhysicalRowDataType();

        ReadableConfig options = helper.getOptions();
        SDBSourceOptions sourceOptions = new SDBSourceOptions(options);

        return new SDBDynamicTableSource(sourceOptions, produceDatatype);
    }

}