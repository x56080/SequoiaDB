/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = CollectionSnapshotRecord.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.model;

import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.common.FieldName;
import lombok.Data;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;

@Data
public class CollectionSnapshotRecord {

    private String collection;
    private String site;
    private long lastRecordTime;
    private BasicBSONList snapshots;
    private boolean recordSnapshotEffective;
    private boolean lobSnapshotEffective;

    public static CollectionSnapshotRecord fromBSONObject(BSONObject obj) {
        CollectionSnapshotRecord record = new CollectionSnapshotRecord();
        record.setCollection(BsonUtils.getStringChecked(obj,
                FieldName.CollectionSnapshotRecord.FIELD_COLLECTION_NAME));
        record.setSite(BsonUtils.getStringChecked(obj,
                FieldName.CollectionSnapshotRecord.FIELD_SITE_NAME));
        record.setLastRecordTime(BsonUtils.getLongChecked(obj,
                FieldName.CollectionSnapshotRecord.FIELD_LAST_RECORD_TIME));
        record.setSnapshots(BsonUtils.getArray(obj, FieldName.CollectionSnapshotRecord.FIELD_SNAPSHOTS));
        record.setRecordSnapshotEffective(BsonUtils.getBooleanOrElse(obj, FieldName.CollectionSnapshotRecord.FIELD_RECORD_SNAPSHOT_EFFECTIVE, false));
        record.setLobSnapshotEffective(BsonUtils.getBooleanOrElse(obj, FieldName.CollectionSnapshotRecord.FIELD_LOB_SNAPSHOT_EFFECTIVE, false));
        return record;
    }

}
