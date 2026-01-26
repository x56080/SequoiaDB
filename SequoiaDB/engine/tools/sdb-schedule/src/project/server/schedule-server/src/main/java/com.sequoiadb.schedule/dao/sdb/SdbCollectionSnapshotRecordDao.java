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

   Source File Name = SdbCollectionSnapshotRecordDao.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.dao.sdb;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.dao.CollectionSnapshotRecordDao;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
import com.sequoiadb.schedule.metasource.template.DataSourceWrapper;
import com.sequoiadb.schedule.metasource.template.ITransaction;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplate;
import com.sequoiadb.schedule.metasource.template.SequoiadbTransaction;
import com.sequoiadb.schedule.model.CollectionSnapshotRecord;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import javax.annotation.PostConstruct;

@Repository
public class SdbCollectionSnapshotRecordDao implements CollectionSnapshotRecordDao {
    private static final Logger logger = LoggerFactory
            .getLogger(SdbCollectionSnapshotRecordDao.class);

    private SequoiadbTemplate template;

    private static final String CL_COLLECTION_SNAPSHOT_RECORD = "COLLECTION_SNAPSHOT_RECORD";
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;
    private static final String INDEX_NAME = "idx_collection_site";

    @Autowired
    public SdbCollectionSnapshotRecordDao(DataSourceWrapper dataSourceWrapper) {
        this.template = new SequoiadbTemplate(dataSourceWrapper);
    }

    @PostConstruct
    public void init() {
        try {
            ensureTableAndIndex();
        }
        catch (Exception e) {
            logger.error("failed to create table or index", e);
            asyncRetryEnsureTableAndIndex();
        }
    }

    private void asyncRetryEnsureTableAndIndex() {
        if (retryTimer == null) {
            retryTimer = TimerFactory.createTimer("retryEnsureTableAndIndexTimer");
        }
        retryTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    logger.info("retry to create table and index");
                    ensureTableAndIndex();
                    cancel();
                }
                catch (Exception e) {
                    logger.error("failed to create table or index", e);
                }
            }
        }, RETRY_INTERVAL, RETRY_INTERVAL);
    }

    private void ensureTableAndIndex() throws ScheduleServerMetaSourceException {
        ensureTable();
        ensureIndex();
    }

    private void ensureTable() throws ScheduleServerMetaSourceException {
        try {
            template.collectionSpace().createCollection(CL_COLLECTION_SNAPSHOT_RECORD);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new ScheduleServerMetaSourceException("failed to create collection:"
                        + template.getSystemCSName() + "." + CL_COLLECTION_SNAPSHOT_RECORD, e);
            }
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to create collection:"
                    + template.getSystemCSName() + "." + CL_COLLECTION_SNAPSHOT_RECORD, e);
        }
    }

    private void ensureIndex() throws ScheduleServerMetaSourceException {
        BasicBSONObject indexBson = new BasicBSONObject();
        indexBson.put(FieldName.CollectionSnapshotRecord.FIELD_SITE_NAME, 1);
        indexBson.put(FieldName.CollectionSnapshotRecord.FIELD_COLLECTION_NAME, 1);
        try {
            template.collection(CL_COLLECTION_SNAPSHOT_RECORD).ensureIndex(INDEX_NAME, indexBson,
                    true);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create index:csName=" + template.getSystemCSName() + ",clName="
                            + CL_COLLECTION_SNAPSHOT_RECORD + ",index=" + INDEX_NAME,
                    e);
        }
    }

    @Override
    public CollectionSnapshotRecord findOne(String siteName, String clFullName) throws Exception {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.CollectionSnapshotRecord.FIELD_SITE_NAME, siteName);
        matcher.put(FieldName.CollectionSnapshotRecord.FIELD_COLLECTION_NAME, clFullName);
        BSONObject object = template.collection(CL_COLLECTION_SNAPSHOT_RECORD).findOne(matcher,
                null);
        if (object != null) {
            return CollectionSnapshotRecord.fromBSONObject(object);
        }
        return null;
    }

    @Override
    public void upsert(String siteName, String clFullName, BSONObject snapshot,
            boolean recordSnapshotEffective, boolean lobSnapshotEffective) throws Exception {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.CollectionSnapshotRecord.FIELD_SITE_NAME, siteName);
        matcher.put(FieldName.CollectionSnapshotRecord.FIELD_COLLECTION_NAME, clFullName);
        BSONObject newValue = new BasicBSONObject();
        newValue.put(FieldName.CollectionSnapshotRecord.FIELD_SITE_NAME, siteName);
        newValue.put(FieldName.CollectionSnapshotRecord.FIELD_COLLECTION_NAME, clFullName);
        newValue.put(FieldName.CollectionSnapshotRecord.FIELD_SNAPSHOT, snapshot);
        newValue.put(FieldName.CollectionSnapshotRecord.FIELD_LAST_RECORD_TIME,
                System.currentTimeMillis());
        newValue.put(FieldName.CollectionSnapshotRecord.FIELD_RECORD_SNAPSHOT_EFFECTIVE,
                recordSnapshotEffective);
        newValue.put(FieldName.CollectionSnapshotRecord.FIELD_LOB_SNAPSHOT_EFFECTIVE,
                lobSnapshotEffective);
        BSONObject modifier = new BasicBSONObject("$set", newValue);
        template.collection(CL_COLLECTION_SNAPSHOT_RECORD).upsert(matcher, modifier);
    }

    @Override
    public void delete(String siteName, String clFullName, ITransaction t) throws Exception {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.CollectionSnapshotRecord.FIELD_SITE_NAME, siteName);
        matcher.put(FieldName.CollectionSnapshotRecord.FIELD_COLLECTION_NAME, clFullName);
        template.collection(CL_COLLECTION_SNAPSHOT_RECORD).delete(matcher,
                (SequoiadbTransaction) t);
    }

    @Override
    public void updateRecordSnapshotEffective(String siteName, String clFullName, boolean recordSnapshotEffective) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.CollectionSnapshotRecord.FIELD_SITE_NAME, siteName);
        matcher.put(FieldName.CollectionSnapshotRecord.FIELD_COLLECTION_NAME, clFullName);
        BSONObject newValue = new BasicBSONObject();
        newValue.put(FieldName.CollectionSnapshotRecord.FIELD_RECORD_SNAPSHOT_EFFECTIVE,
                recordSnapshotEffective);
        BSONObject modifier = new BasicBSONObject("$set", newValue);
        template.collection(CL_COLLECTION_SNAPSHOT_RECORD).update(matcher, modifier);
    }

    @Override
    public void updateLobSnapshotEffective(String siteName, String clFullName, boolean lobSnapshotEffective) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.CollectionSnapshotRecord.FIELD_SITE_NAME, siteName);
        matcher.put(FieldName.CollectionSnapshotRecord.FIELD_COLLECTION_NAME, clFullName);
        BSONObject newValue = new BasicBSONObject();
        newValue.put(FieldName.CollectionSnapshotRecord.FIELD_LOB_SNAPSHOT_EFFECTIVE,
                lobSnapshotEffective);
        BSONObject modifier = new BasicBSONObject("$set", newValue);
        template.collection(CL_COLLECTION_SNAPSHOT_RECORD).update(matcher, modifier);
    }
}
