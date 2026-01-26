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

   Source File Name = CollectionDataSwitchEventChecker.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.common.SdbHelper;
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.core.elect.LeaderElect;
import com.sequoiadb.schedule.core.task.SdbDataSwitchTask;
import com.sequoiadb.schedule.dao.CollectionDataSwitchEventDao;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleSystemException;
import com.sequoiadb.schedule.lock.TransactionLock;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.model.CollectionDataSwitchEvent;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;

@Component
public class CollectionDataSwitchEventChecker {
    private static final Logger logger = LoggerFactory.getLogger(CollectionDataSwitchEventChecker.class);
    private static final int CHECK_INTERVAL_MS = 2 * 60 * 1000; // 2 minutes

    @Autowired
    private CollectionDataSwitchEventDao collectionDataSwitchEventDao;

    @Autowired
    private LeaderElect leaderElect;

    @Autowired
    private SiteDataserviceMgr siteDataserviceMgr;

    private Timer checkTimer;

    @PostConstruct
    public void init() {
        if (checkTimer == null) {
            checkTimer = TimerFactory.createTimer("CollectionDataSwitchEventCheckerTimer");
        }
        checkTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    if (!leaderElect.isLeader()) {
                        return;
                    }
                    doCheck();
                }
                catch (Exception e) {
                    logger.warn("do check collection dataSwitch event failed", e);
                }
            }
        }, 0, CHECK_INTERVAL_MS);
    }

    private void doCheck() throws Exception {
        MetaCursor cursor = null;
        try {
            cursor = collectionDataSwitchEventDao.query(
                    new BasicBSONObject(FieldName.CollectionDataSwitchEvent.FIELD_STATUS,
                            ScheduleDefine.CollectionDataSwitchStatus.DATA_SWITCHING));
            while (cursor.hasNext()) {
                BSONObject object = cursor.getNext();
                CollectionDataSwitchEvent event = CollectionDataSwitchEvent.fromBSONObject(object);
                TransactionLock clLock = null;
                try {
                    clLock = createCollectionTransactionLock(event.getCollection());
                    clLock.lock();
                    handleCollectionDataSwitch(event);
                    updateEventStatusToDataSwitched(event);
                }
                catch (Exception e) {
                    logger.warn("check collection data switch event failed, event: {}", event.toBSONObject(), e);
                }
                finally {
                    if (clLock != null) {
                        clLock.unlock();
                    }
                }
            }
        }
        finally {
            if (null != cursor) {
                cursor.close();
            }
        }
    }

    private TransactionLock createCollectionTransactionLock(String clName) throws ScheduleServerException {
        try {
            return ScheduleServer.getInstance()
                    .createTransactionLock(ScheduleDefine.TransactionLockKey.LOCK_COLLECTION, clName);
        }
        catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_TIMEOUT.getErrorCode()) {
                throw new ScheduleServerException(ScheduleServerError.TRANSACTION_LOCK_TIMEOUT,
                        "failed to create collection transaction lock, cl: " + clName, e);
            }
            throw new ScheduleSystemException("failed to create collection transaction lock, cl: " + clName, e);
        }
    }

    private void handleCollectionDataSwitch(CollectionDataSwitchEvent event) throws Exception {
        String sourceSite = event.getSourceSite();
        DataServiceWrapper sourceDataService = siteDataserviceMgr.getDataService(sourceSite);
        Sequoiadb sdb = null;
        try {
            sdb = sourceDataService.getConnection();
            String clName = event.getCollection();
            BSONObject currentClCataInfo = SdbHelper.getCataSnapshotByClName(sdb, clName);

            BSONObject sourceClCataInfo = event.getSourceClCataInfo();
            String mainCLName = BsonUtils.getString(sourceClCataInfo, "MainCLName");
            boolean isSubCl = mainCLName != null && !mainCLName.isEmpty();
            BSONObject attachInfo = event.getSourceClAttachInfo();

            if (currentClCataInfo != null) {
                boolean isMapDs = currentClCataInfo.containsField("DataSourceID");
                if (isMapDs) {
                    if (!isSubCl) {
                        // 普通表，直接结束
                        logger.info("cl is already map to data source, update event status to data_switched, event: {}",
                                event.toBSONObject());
                    }
                    else {
                        // 主子表，检查是否挂载
                        boolean alreadyAttach = currentClCataInfo.containsField("MainCLName");
                        if (!alreadyAttach) {
                            doAttach(sdb, clName, mainCLName, sourceSite, attachInfo);
                        }
                        else {
                            logger.info("cl is already map to data source and attached, update event status to data_switched, event: {}",
                                    event.toBSONObject());
                        }
                    }
                    return;
                }

                if (isSubCl) {
                    logger.info("detach sub cl {} from main cl {}, site: {}", clName, mainCLName, sourceSite);
                    SdbHelper.detachCl(sdb, mainCLName, clName);
                }

                String rename = event.getRename();

                logger.info("rename cl {} to {}, site: {}", clName, rename, sourceSite);
                SdbHelper.renameCl(sdb, clName, rename);

                createAndAttachCL(sdb, clName, mainCLName, isSubCl, attachInfo, event.getTargetDatasource(), sourceSite);
            }
            else {
                createAndAttachCL(sdb, clName, mainCLName, isSubCl, attachInfo, event.getTargetDatasource(), sourceSite);
            }
        }
        finally {
            if (sdb != null) {
                sourceDataService.releaseConnection(sdb);
            }
        }
    }

    private void createAndAttachCL(Sequoiadb sdb, String clName, String mainCLName, boolean isSubCl, BSONObject attachInfo, String dataSource, String sourceSite) throws Exception {
        logger.info("create cl and map to data source, cl: {}, site: {}, datasource: {}", clName, sourceSite, dataSource);
        SdbDataSwitchTask.createCLMapToDS(sdb, clName, dataSource);

        if (isSubCl) {
            doAttach(sdb, clName, mainCLName, sourceSite, attachInfo);
        }
    }

    private void doAttach(Sequoiadb sdb, String clName, String mainCLName, String sourceSite, BSONObject attachInfo)  throws Exception {
        logger.info("attach sub cl {} to main cl {}, site: {}, attachInfo: {}", clName, mainCLName, sourceSite,
                attachInfo);
        SdbHelper.attachCl(sdb, mainCLName, clName,
                (BSONObject) attachInfo.get("LowBound"),
                (BSONObject) attachInfo.get("UpBound"));
    }

    private void updateEventStatusToDataSwitched(CollectionDataSwitchEvent event) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_COLLECTION_NAME, event.getCollection());
        matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_SITE_NAME, event.getSourceSite());
        matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_TARGET_SITE_NAME, event.getTargetSite());
        matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_TARGET_DATASOURCE_NAME, event.getTargetDatasource());
        matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_CL_RENAME, event.getRename());

        BSONObject newValue = new BasicBSONObject(FieldName.CollectionDataSwitchEvent.FIELD_STATUS, ScheduleDefine.CollectionDataSwitchStatus.DATA_SWITCHED);
        newValue.put(FieldName.CollectionDataSwitchEvent.FIELD_DATA_SWITCHED_TIME, System.currentTimeMillis());
        BSONObject modifier = new BasicBSONObject("$set", newValue);
        try {
            collectionDataSwitchEventDao.update(matcher, modifier);
        }
        catch (Exception e) {
            logger.warn("update collection data switch event status failed, event: {}, newStatus: {}",
                    event.toBSONObject(), ScheduleDefine.CollectionDataSwitchStatus.DATA_SWITCHED, e);
        }
    }

    @PreDestroy
    public void destroy() {
        if (checkTimer != null) {
            checkTimer.cancel();
            checkTimer = null;
        }
    }
}
