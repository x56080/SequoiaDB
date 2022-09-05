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

package com.sequoiadb.flink.format.json.cgb;

import com.sequoiadb.flink.common.exception.SDBException;
import org.apache.flink.api.common.serialization.DeserializationSchema;
import org.apache.flink.api.common.typeinfo.TypeInformation;
import org.apache.flink.formats.common.TimestampFormat;
import org.apache.flink.formats.json.JsonRowDataDeserializationSchema;
import org.apache.flink.shaded.jackson2.com.fasterxml.jackson.databind.JsonNode;
import org.apache.flink.shaded.jackson2.com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.flink.table.api.DataTypes;
import org.apache.flink.table.data.GenericRowData;
import org.apache.flink.table.data.RowData;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.logical.RowType;
import org.apache.flink.table.types.utils.DataTypeUtils;
import org.apache.flink.types.RowKind;
import org.apache.flink.util.Collector;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.Serializable;
import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

import static com.sequoiadb.flink.format.json.cgb.CGBCanalJsonDecodingFormat.ReadableMetadata;

public class CGBCanalJsonDeserializationSchema implements DeserializationSchema<RowData> {

    private static final Logger LOG = LoggerFactory.getLogger(CGBCanalJsonDeserializationSchema.class);

    private static final long serialVersionUID = 1L;

    // supported op type
    private static final String OP_INSERT = "INSERT";
    private static final String OP_UPDATE = "UPDATE";
    private static final String OP_DELETE = "DELETE";

    // cgb technical field
    private static final String TYPE = "__type";
    private static final String BEFORE = "__before";

    /** The deserializer to deserialize CGB Canal JSON data. */
    private final JsonRowDataDeserializationSchema jsonDeserializer;
    private final JsonRowDataDeserializationSchema rowDeserializer;

    /** Flag that indicates that an additional projection is required for metadata. */
    private final boolean hasMetadata;

    private final MetadataConverter[] metadataConverters;

    private final TypeInformation<RowData> producedTypeInfo;

    private final boolean ignoreParseErrors;
    private final int[] upsertKeyPositions;

    private final RowType jsonRowType;
    private final DataType physicalDataType;

    private final ObjectMapper objectMapper = new ObjectMapper();

    public CGBCanalJsonDeserializationSchema(
            DataType physicalDataType,
            List<ReadableMetadata> requestedMetadata,
            TypeInformation<RowData> producedTypeInfo,
            boolean ignoreParseErrors,
            TimestampFormat timestampFormat,
            String[] upsertKey) {
        this.jsonRowType = createJsonRowType(physicalDataType, requestedMetadata);
        this.physicalDataType = physicalDataType;
        this.jsonDeserializer =
                new JsonRowDataDeserializationSchema(
                        jsonRowType,
                        producedTypeInfo,
                        false,
                        ignoreParseErrors,
                        timestampFormat);
        // row deserializer for __before field (json string)
        final RowType physicalRowType = (RowType) physicalDataType.getLogicalType();
        this.rowDeserializer =
                new JsonRowDataDeserializationSchema(
                        physicalRowType,
                        producedTypeInfo,
                        false,
                        ignoreParseErrors,
                        timestampFormat);
        this.hasMetadata = requestedMetadata.size() > 0;
        this.metadataConverters = createMetadataConverters(jsonRowType, requestedMetadata);
        this.producedTypeInfo = producedTypeInfo;
        this.ignoreParseErrors = ignoreParseErrors;

        this.upsertKeyPositions = new int[upsertKey.length];
        for (int i = 0; i < upsertKey.length; i++) {
            int pos = findFieldPosByName(upsertKey[i], physicalRowType);
            if (pos == -1) {
                throw new SDBException(
                        String.format("can't match primary key: %s in schema: %s.",
                                upsertKey[i],
                                physicalRowType));
            }
            upsertKeyPositions[i] = pos;
        }
    }

    private static RowType createJsonRowType(
            DataType physicalDataType, List<ReadableMetadata> readableMetadata) {
        // append fields that are required for reading metadata in the root
        final List<DataTypes.Field> rootFields =
                readableMetadata.stream()
                        .map(m -> m.requiredJsonField) // add required metadata field
                        .filter(m -> !m.getName().equals(TYPE))
                        .distinct()
                        .collect(Collectors.toList());
        // add cgb cdc changelog's technical fields that needed.
        // '__before' field in changelog is a json string.
        rootFields.add(DataTypes.FIELD(BEFORE, DataTypes.STRING()));
        rootFields.add(DataTypes.FIELD(TYPE, DataTypes.STRING()));

        return (RowType) DataTypeUtils.appendRowFields(physicalDataType, rootFields).getLogicalType();
    }

    private static MetadataConverter[] createMetadataConverters(
            RowType jsonRowType, List<ReadableMetadata> requestedMetadata) {
        return requestedMetadata.stream()
                .map(m -> convertInRoot(jsonRowType, m))
                .toArray(MetadataConverter[]::new);
    }

    private static MetadataConverter convertInRoot(RowType jsonRowType, ReadableMetadata metadata) {
        final int pos = findFieldPosByMetadata(metadata, jsonRowType);
        return new MetadataConverter() {
            @Override
            public Object convert(GenericRowData root, int unused) {
                if (pos == -1) {
                    return null;
                }
                return metadata.converter.convert(root, pos);
            }
        };
    }

    private static int findFieldPosByMetadata(ReadableMetadata metadata, RowType rowType) {
        return rowType.getFieldNames().indexOf(metadata.requiredJsonField.getName());
    }

    private static int findFieldPosByName(String fieldName, RowType rowType) {
        return rowType.getFieldNames().indexOf(fieldName);
    }

    @Override
    public RowData deserialize(byte[] message) throws IOException {
        throw new RuntimeException(
                "Please invoke DeserializationSchema#deserialize(byte[], Collector<RowData>) instead.");
    }

    @Override
    public void deserialize(byte[] message, Collector<RowData> out) throws IOException {
        if (message == null || message.length == 0) {
            return;
        }

        // get data from json root
        try {
            GenericRowData row = (GenericRowData) jsonDeserializer.deserialize(message);
            GenericRowData before = null;

            Object beforeJson = row.getField(findFieldPosByName(BEFORE, jsonRowType));
            if (beforeJson != null) {
                JsonNode jsonNode = objectMapper.readTree(beforeJson.toString());
                before = (GenericRowData) rowDeserializer.convertToRowData(jsonNode);
            }

            GenericRowData after = convertRootRowInAfter(row, physicalDataType);
            String op = row.getField(findFieldPosByName(TYPE, jsonRowType)).toString();

            if (OP_INSERT.equals(op)) {
                after.setRowKind(RowKind.INSERT);
                emitRow(row, null, after, out);
            } else if (OP_UPDATE.equals(op)) {
                before.setRowKind(RowKind.UPDATE_BEFORE);
                after.setRowKind(RowKind.UPDATE_AFTER);
                emitRow(row, before, after, out);
            } else if (OP_DELETE.equals(op)) {
                after.setRowKind(RowKind.DELETE);
                emitRow(row, null, after, out);
            } else {
                if (!ignoreParseErrors) {
                    throw new IOException(
                            String.format(
                                    "unknown \"__type\" value \"%s\".", op));
                }
            }
        } catch (Throwable cause) {
            // a big try-catch to protect the processing.
            if (!ignoreParseErrors) {
                throw new IOException("corrupt cgb json message.", cause);
            }
        }
    }

    // --------------------------------------------------------------------------------------------

    private void emitRow(GenericRowData rootRow,
            GenericRowData before, GenericRowData after, Collector<RowData> out) throws IOException {
        GenericRowData producedAfter = convertToFinalRow(rootRow, after);

        if (before != null && hasPriKeyChanges(before)) {
            fillPriKeyInBefore(before, after);
            GenericRowData producedBefore = convertToFinalRow(rootRow, before);
            out.collect(producedBefore);
        }

        out.collect(producedAfter);
    }

    private GenericRowData convertToFinalRow(GenericRowData rootRow, GenericRowData physicalRow) {
        if (!hasMetadata) {
            return physicalRow;
        }

        final int physicalArity = physicalRow.getArity();
        final int metadataArity = metadataConverters.length;

        final GenericRowData producedRow =
                new GenericRowData(physicalRow.getRowKind(), physicalArity + metadataArity);

        for (int physicalPos = 0; physicalPos < physicalArity; physicalPos++) {
            producedRow.setField(physicalPos, physicalRow.getField(physicalPos));
        }

        for (int metadataPos = 0; metadataPos < metadataArity; metadataPos++) {
            producedRow.setField(
                    physicalArity + metadataPos, metadataConverters[metadataPos].convert(rootRow));
        }

        return producedRow;
    }

    private GenericRowData convertRootRowInAfter(GenericRowData rootRow, DataType physicalDataType) {
        RowType rowType = (RowType) physicalDataType.getLogicalType();

        GenericRowData rowData = new GenericRowData(rowType.getFieldCount());
        for (int pos = 0; pos < rowData.getArity(); pos++) {
            rowData.setField(pos, rootRow.getField(pos));
        }
        return rowData;
    }

    private boolean hasPriKeyChanges(GenericRowData before) {
        for (int upsertKeyPosition : upsertKeyPositions) {
            if (!before.isNullAt(upsertKeyPosition)) {
                return true;
            }
        }
        return false;
    }

    private void fillPriKeyInBefore(
            GenericRowData before, GenericRowData after) {
        for (int upsertKeyPosition : upsertKeyPositions) {
            if (before.isNullAt(upsertKeyPosition)) {
                before.setField(upsertKeyPosition,
                        after.getField(upsertKeyPosition));
            }
        }
    }

    @Override
    public boolean isEndOfStream(RowData nextElement) {
        return false;
    }

    @Override
    public TypeInformation<RowData> getProducedType() {
        return producedTypeInfo;
    }

    @Override
    public boolean equals(Object anObj) {
        if (this == anObj) {
            return true;
        }
        if (anObj == null || getClass() != anObj.getClass()) {
            return false;
        }
        CGBCanalJsonDeserializationSchema that = (CGBCanalJsonDeserializationSchema) anObj;
        return Objects.equals(jsonDeserializer, that.jsonDeserializer)
                && hasMetadata == that.hasMetadata
                && Objects.equals(producedTypeInfo, that.producedTypeInfo)
                && ignoreParseErrors == that.ignoreParseErrors;
    }

    @Override
    public int hashCode() {
        return Objects.hash(jsonDeserializer, hasMetadata, producedTypeInfo, ignoreParseErrors);
    }

    interface MetadataConverter extends Serializable {

        default Object convert(GenericRowData row) {
            return convert(row, -1);
        }

        Object convert(GenericRowData row, int pos);
    }

}