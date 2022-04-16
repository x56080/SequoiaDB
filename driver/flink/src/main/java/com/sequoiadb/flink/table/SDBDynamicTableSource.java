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

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import com.sequoiadb.flink.codec.SDBDataConverter;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.source.SDBSource;

import org.apache.flink.table.connector.ChangelogMode;
import org.apache.flink.table.connector.source.*;
import org.apache.flink.table.connector.source.abilities.SupportsLimitPushDown;
import org.apache.flink.table.connector.source.abilities.SupportsProjectionPushDown;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.FieldsDataType;
import org.apache.flink.table.types.logical.LogicalType;
import org.apache.flink.table.types.logical.RowType;
import org.apache.flink.types.RowKind;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SDBDynamicTableSource implements ScanTableSource,
        SupportsProjectionPushDown,
        SupportsLimitPushDown {

    private static final Logger LOG = LoggerFactory.getLogger(SDBDynamicTableSource.class);

    private final SDBSourceOptions sourceOptions;

    private DataType producedDatatype;
    private long limit = -1;

    public SDBDynamicTableSource(SDBSourceOptions sourceOptions,
                                 DataType produceDatatype) {
        LOG.info("source options: {}", sourceOptions);

        this.sourceOptions = sourceOptions;
        this.producedDatatype = produceDatatype;
    }

    @Override
    public DynamicTableSource copy() {
        return new SDBDynamicTableSource(sourceOptions, producedDatatype);
    }

    @Override
    public String asSummaryString() {
        return "SequoiaDB Dynamic Table Source";
    }

    @Override
    public ChangelogMode getChangelogMode() {
        return ChangelogMode.newBuilder()
                .addContainedKind(RowKind.INSERT)
                .build();
    }

    @Override
    public ScanRuntimeProvider getScanRuntimeProvider(ScanContext runtimeProviderContext) {
        SDBDataConverter dataConverter
                = new SDBDataConverter(((RowType) producedDatatype.getLogicalType()));
        return SourceProvider.of(new SDBSource(
                dataConverter,
                sourceOptions,
                ((RowType) producedDatatype.getLogicalType()).getFieldNames(),
                limit));
    }

    @Override
    public void applyLimit(long limit) {
        this.limit = limit;
    }

    @Override
    public boolean supportsNestedProjection() {
        return false;
    }

    @Override
    public void applyProjection(int[][] requiredColumns) {
        final List<RowType.RowField> updatedFields = new ArrayList<>();
        final List<DataType> updatedChildren = new ArrayList<>();
        Set<String> nameDomain = new HashSet<>();

        int duplicateCount = 0;
        DataType dataType = producedDatatype;
        for (int[] requiredColumn : requiredColumns) {
            DataType fieldType = dataType.getChildren().get(requiredColumn[0]);
            LogicalType fieldLogicalType = fieldType.getLogicalType();

            StringBuilder builder =
                    new StringBuilder(((RowType) dataType.getLogicalType())
                            .getFieldNames()
                            .get(requiredColumn[0]));

            for (int i = 1; i < requiredColumn.length; i++) {
                RowType rowType = (RowType) fieldLogicalType;
                builder.append("_").append(rowType.getFieldNames().get(requiredColumn[i]));
                fieldLogicalType = rowType.getFields().get(requiredColumn[i]).getType();
                fieldType = fieldType.getChildren().get(requiredColumn[i]);
            }

            String path = builder.toString();
            while (nameDomain.contains(path)) {
                path = builder.append("_$").append(duplicateCount++).toString();
            }
            updatedFields.add(new RowType.RowField(path, fieldLogicalType));
            updatedChildren.add(fieldType);
            nameDomain.add(path);
        }

        this.producedDatatype = new FieldsDataType(
                new RowType(dataType.getLogicalType().isNullable(), updatedFields),
                dataType.getConversionClass(),
                updatedChildren
                );
    }

}