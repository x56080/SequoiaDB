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

   Source File Name = SdbCollectionDataSwitchEventDao.java

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
import com.sequoiadb.schedule.dao.CollectionDataSwitchEventDao;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.metasource.template.DataSourceWrapper;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplate;
import com.sequoiadb.schedule.model.CollectionDataSwitchEvent;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import javax.annotation.PostConstruct;

@Repository
public class SdbCollectionDataSwitchEventDao implements CollectionDataSwitchEventDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbCollectionDataSwitchEventDao.class);

    private SequoiadbTemplate template;

    private static final String CL_COLLECTION_DATA_SWITCH_EVENT = "COLLECTION_DATA_SWITCH_EVENT";
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;
    private static final String INDEX_NAME = "idx_collection_site";

    @Autowired
    public SdbCollectionDataSwitchEventDao(DataSourceWrapper dataSourceWrapper) {
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
            template.collectionSpace().createCollection(CL_COLLECTION_DATA_SWITCH_EVENT);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new ScheduleServerMetaSourceException("failed to create collection:"
                        + template.getSystemCSName() + "." + CL_COLLECTION_DATA_SWITCH_EVENT, e);
            }
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to create collection:"
                    + template.getSystemCSName() + "." + CL_COLLECTION_DATA_SWITCH_EVENT, e);
        }
    }

    private void ensureIndex() throws ScheduleServerMetaSourceException {
        BasicBSONObject indexBson = new BasicBSONObject();
        indexBson.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_SITE_NAME, 1);
        indexBson.put(FieldName.CollectionDataSwitchEvent.FIELD_COLLECTION_NAME, 1);
        indexBson.put(FieldName.CollectionDataSwitchEvent.FIELD_TARGET_SITE_NAME, 1);
        try {
            template.collection(CL_COLLECTION_DATA_SWITCH_EVENT).ensureIndex(INDEX_NAME, indexBson, true);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create index:csName=" + template.getSystemCSName() + ",clName="
                            + CL_COLLECTION_DATA_SWITCH_EVENT + ",index=" + INDEX_NAME,
                    e);
        }
    }

    @Override
    public void insert(CollectionDataSwitchEvent event) throws Exception {
        BSONObject obj = event.toBSONObject();
        template.collection(CL_COLLECTION_DATA_SWITCH_EVENT).insert(obj);
    }

    @Override
    public void update(BSONObject matcher, BSONObject modifier) throws Exception {
        template.collection(CL_COLLECTION_DATA_SWITCH_EVENT).update(matcher, modifier);
    }

    @Override
    public MetaCursor query(BSONObject matcher) throws Exception {
        return template.collection(CL_COLLECTION_DATA_SWITCH_EVENT).find(matcher);
    }
}
