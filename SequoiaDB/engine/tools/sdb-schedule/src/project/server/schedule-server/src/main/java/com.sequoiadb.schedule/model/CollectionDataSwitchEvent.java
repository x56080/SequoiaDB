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

   Source File Name = CollectionDataSwitchEvent.java

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
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class CollectionDataSwitchEvent {

    private String collection;
    private String sourceSite;
    private String targetSite;
    private String targetDatasource;
    private BSONObject sourceClCataInfo;
    private BSONObject sourceClAttachInfo;
    private String rename;
    private String status;
    private Long dataSwitchedTime;

    public CollectionDataSwitchEvent(String collection, String sourceSite, String targetSite,
            String targetDatasource, BSONObject sourceClCataInfo, BSONObject sourceClAttachInfo,
            String rename, String status) {
        this(collection, sourceSite, targetSite, targetDatasource,
                sourceClCataInfo, sourceClAttachInfo, rename, status, null);
    }

    public CollectionDataSwitchEvent(String collection, String sourceSite, String targetSite,
            String targetDatasource, BSONObject sourceClCataInfo, BSONObject sourceClAttachInfo,
            String rename, String status, Long dataSwitchedTime) {
        this.collection = collection;
        this.sourceSite = sourceSite;
        this.targetSite = targetSite;
        this.targetDatasource = targetDatasource;
        this.sourceClCataInfo = sourceClCataInfo;
        this.sourceClAttachInfo = sourceClAttachInfo;
        this.rename = rename;
        this.status = status;
        this.dataSwitchedTime = dataSwitchedTime;
    }

    public static CollectionDataSwitchEvent fromBSONObject(BSONObject obj) {
        String collection = BsonUtils.getStringChecked(obj,
                FieldName.CollectionDataSwitchEvent.FIELD_COLLECTION_NAME);
        String sourceSite = BsonUtils.getStringChecked(obj,
                FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_SITE_NAME);
        String targetSite = BsonUtils.getStringChecked(obj,
                FieldName.CollectionDataSwitchEvent.FIELD_TARGET_SITE_NAME);
        String targetDatasource = BsonUtils.getStringChecked(obj,
                FieldName.CollectionDataSwitchEvent.FIELD_TARGET_DATASOURCE_NAME);
        BSONObject cataInfo = BsonUtils.getBSONObjectChecked(obj,
                FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_CL_CATA_INFO);
        BSONObject attachInfo = BsonUtils.getBSON(obj,
                FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_CL_ATTACH_INFO);
        String rename = BsonUtils.getStringChecked(obj,
                FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_CL_RENAME);
        String status = BsonUtils.getString(obj,
                FieldName.CollectionDataSwitchEvent.FIELD_STATUS);
        Long dataSwitchedTime = BsonUtils.getLong(obj,
                FieldName.CollectionDataSwitchEvent.FIELD_DATA_SWITCHED_TIME);
        return new CollectionDataSwitchEvent(collection, sourceSite, targetSite, targetDatasource,
                cataInfo, attachInfo, rename, status, dataSwitchedTime);
    }

    public BSONObject toBSONObject() {
        BSONObject obj = new BasicBSONObject();
        obj.put(FieldName.CollectionDataSwitchEvent.FIELD_COLLECTION_NAME, collection);
        obj.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_SITE_NAME, sourceSite);
        obj.put(FieldName.CollectionDataSwitchEvent.FIELD_TARGET_SITE_NAME, targetSite);
        obj.put(FieldName.CollectionDataSwitchEvent.FIELD_TARGET_DATASOURCE_NAME, targetDatasource);
        obj.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_CL_CATA_INFO, sourceClCataInfo);
        obj.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_CL_ATTACH_INFO, sourceClAttachInfo);
        obj.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_CL_RENAME, rename);
        obj.put(FieldName.CollectionDataSwitchEvent.FIELD_STATUS, status);
        if (dataSwitchedTime != null) {
            obj.put(FieldName.CollectionDataSwitchEvent.FIELD_DATA_SWITCHED_TIME, dataSwitchedTime);
        }
        return obj;
    }

    public String getCollection() {
        return collection;
    }

    public String getSourceSite() {
        return sourceSite;
    }

    public String getTargetSite() {
        return targetSite;
    }

    public String getTargetDatasource() {
        return targetDatasource;
    }

    public BSONObject getSourceClCataInfo() {
        return sourceClCataInfo;
    }

    public BSONObject getSourceClAttachInfo() {
        return sourceClAttachInfo;
    }

    public String getRename() {
        return rename;
    }

    public Long getDataSwitchedTime() {
        return dataSwitchedTime;
    }
}
