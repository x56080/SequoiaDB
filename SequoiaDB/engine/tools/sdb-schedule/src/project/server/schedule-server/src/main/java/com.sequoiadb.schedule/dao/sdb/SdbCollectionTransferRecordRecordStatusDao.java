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

   Source File Name = SdbCollectionTransferRecordRecordStatusDao.java

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
import com.sequoiadb.schedule.dao.CollectionTransferRecordStatusDao;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
import com.sequoiadb.schedule.metasource.template.DataSourceWrapper;
import com.sequoiadb.schedule.metasource.template.ITransaction;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplate;
import com.sequoiadb.schedule.metasource.template.SequoiadbTransaction;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import javax.annotation.PostConstruct;

@Repository
public class SdbCollectionTransferRecordRecordStatusDao implements CollectionTransferRecordStatusDao {
    private static final Logger logger = LoggerFactory
            .getLogger(CollectionTransferRecordStatusDao.class);
    private SequoiadbTemplate template;

    private static final String CL_COLLECTION_TRANSFER_RECORD_STATUS = "COLLECTION_TRANSFER_RECORD_STATUS";
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;
    private static final String INDEX_NAME = "idx_collection_site";

    @Autowired
    public SdbCollectionTransferRecordRecordStatusDao(DataSourceWrapper dataSourceWrapper) {
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
            template.collectionSpace().createCollection(CL_COLLECTION_TRANSFER_RECORD_STATUS);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new ScheduleServerMetaSourceException("failed to create collection:"
                        + template.getSystemCSName() + "." + CL_COLLECTION_TRANSFER_RECORD_STATUS, e);
            }
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to create collection:"
                    + template.getSystemCSName() + "." + CL_COLLECTION_TRANSFER_RECORD_STATUS, e);
        }
    }

    private void ensureIndex() throws ScheduleServerMetaSourceException {
        BasicBSONObject indexBson = new BasicBSONObject();
        indexBson.put(FieldName.CollectionTransferRecordStatus.FIELD_SOURCE_SITE_NAME, 1);
        indexBson.put(FieldName.CollectionTransferRecordStatus.FIELD_TARGET_SITE_NAME, 1);
        indexBson.put(FieldName.CollectionTransferRecordStatus.FIELD_COLLECTION_NAME, 1);
        try {
            template.collection(CL_COLLECTION_TRANSFER_RECORD_STATUS).ensureIndex(INDEX_NAME, indexBson, true);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create index:csName=" + template.getSystemCSName() + ",clName="
                            + CL_COLLECTION_TRANSFER_RECORD_STATUS + ",index=" + INDEX_NAME,
                    e);
        }
    }

    @Override
    public void upsert(String sourceSite, String targetSite, String clName, String startId, String endId) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.CollectionTransferRecordStatus.FIELD_SOURCE_SITE_NAME, sourceSite);
        matcher.put(FieldName.CollectionTransferRecordStatus.FIELD_TARGET_SITE_NAME, targetSite);
        matcher.put(FieldName.CollectionTransferRecordStatus.FIELD_COLLECTION_NAME, clName);
        BSONObject newValue = new BasicBSONObject();
        newValue.put(FieldName.CollectionTransferRecordStatus.FIELD_SOURCE_SITE_NAME, sourceSite);
        newValue.put(FieldName.CollectionTransferRecordStatus.FIELD_TARGET_SITE_NAME, targetSite);
        newValue.put(FieldName.CollectionTransferRecordStatus.FIELD_COLLECTION_NAME, clName);
        newValue.put(FieldName.CollectionTransferRecordStatus.FIELD_RECORD_START_ID, startId);
        newValue.put(FieldName.CollectionTransferRecordStatus.FIELD_RECORD_END_ID,
                endId);
        BSONObject modifier = new BasicBSONObject("$set", newValue);
        template.collection(CL_COLLECTION_TRANSFER_RECORD_STATUS).upsert(matcher, modifier);
    }

    @Override
    public BSONObject queryOne(String sourceSite, String targetSite, String clName) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.CollectionTransferRecordStatus.FIELD_SOURCE_SITE_NAME, sourceSite);
        matcher.put(FieldName.CollectionTransferRecordStatus.FIELD_TARGET_SITE_NAME, targetSite);
        matcher.put(FieldName.CollectionTransferRecordStatus.FIELD_COLLECTION_NAME, clName);
        return template.collection(CL_COLLECTION_TRANSFER_RECORD_STATUS).findOne(matcher);
    }

    @Override
    public void delete(String sourceSite, String targetSite, String clName, ITransaction t) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.CollectionTransferRecordStatus.FIELD_SOURCE_SITE_NAME, sourceSite);
        matcher.put(FieldName.CollectionTransferRecordStatus.FIELD_TARGET_SITE_NAME, targetSite);
        matcher.put(FieldName.CollectionTransferRecordStatus.FIELD_COLLECTION_NAME, clName);
        template.collection(CL_COLLECTION_TRANSFER_RECORD_STATUS).delete(matcher, (SequoiadbTransaction) t);
    }
}
