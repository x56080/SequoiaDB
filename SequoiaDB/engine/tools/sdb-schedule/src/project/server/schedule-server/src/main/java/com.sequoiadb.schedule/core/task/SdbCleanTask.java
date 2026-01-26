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

   Source File Name = SdbCleanTask.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core.task;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.core.DataServiceWrapper;
import com.sequoiadb.schedule.core.ScheduleServer;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.lock.TransactionLock;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.model.SdbCLFullInfo;
import com.sequoiadb.schedule.model.SiteInfo;
import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.util.CollectionUtils;

import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SdbCleanTask extends ScheduleTaskBase {
    private static Logger logger = LoggerFactory.getLogger(SdbCleanTask.class);
    private TransactionLock transactionLock;
    private long taskStartTime = 0;
    private volatile boolean running = true;
    private String taskId;
    private TaskEntity taskEntity;
    private long maxExecTime;
    private BasicBSONList cleanRange;
    private long processClCount = 0;
    private boolean isTimeout = false;

    public SdbCleanTask(ScheduleTaskMgr mgr, TaskEntity taskEntity,
            TransactionLock transactionLock) {
        super(mgr);
        this.transactionLock = transactionLock;
        this.taskEntity = taskEntity;
        this.taskId = taskEntity.getId();
        BSONObject content = taskEntity.getContent();
        maxExecTime = BsonUtils.getLongOrElse(content, FieldName.Schedule.FIELD_MAX_EXEC_TIME, 0);
        cleanRange = BsonUtils.getArray(content, FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE);
    }

    @Override
    public String getName() {
        return "SDB_TASK_CLEAN";
    }

    @Override
    public void _stop() {
        running = false;
    }

    @Override
    public void _runTask() {
        taskStartTime = System.currentTimeMillis();
        try {
            updateTaskRunning(taskId, taskStartTime);
            if (CollectionUtils.isEmpty(cleanRange)) {
                logger.warn("the clean task cleanRange is empty, no clean required, taskId="
                        + getTaskId());
                return;
            }

            Map<String, Map<String, String>> needCleanRenameCl = getNeedCleanRenameCl();
            savePlan(needCleanRenameCl);
            handleClean(needCleanRenameCl);
            // timeout
            if (isTimeout) {
                logger.warn(
                        "task have been interrupted because of timeout:taskId={},startTime={},now={},maxExecTime={}ms",
                        getTaskId(), taskStartTime, new Date(), maxExecTime);
                abortTaskAndAsyncRedo(getTaskId(),
                        ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_TIMEOUT, "timeout",
                        processClCount);
                return;
            }

            // canceled
            int flag = getTaskRunningFlag(getTaskId());
            if (ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_CANCEL == flag) {
                logger.info("task have been canceled:taskId=" + getTaskId());
                stopTaskAndAsyncRedo(getTaskId(), processClCount);
                return;
            }

            // finished
            logger.info("task have been finished:taskId=" + getTaskId());
            finishTaskAndAsyncRedo(getTaskId(), processClCount);
        }
        catch (Exception e) {
            logger.warn("run task failed:taskId=" + getTaskId(), e);
            abortTaskAndAsyncRedo(getTaskId(), ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_ABORT,
                    e.toString(), processClCount);
        }
        finally {
            transactionLock.unlock();
        }
    }

    private void handleClean(Map<String, Map<String, String>> needCleanRenameCl) throws Exception {
        for (Map.Entry<String, Map<String, String>> entry : needCleanRenameCl.entrySet()) {
            String site = entry.getKey();
            Map<String, String> value = entry.getValue();
            if (value == null || value.isEmpty()) {
                continue;
            }

            DataServiceWrapper sourceDataService = ScheduleServer.getInstance()
                    .getDataServiceWrapper(site);
            Sequoiadb sdb = null;
            try {
                sdb = sourceDataService.getConnection();
                for (Map.Entry<String, String> clEntry : value.entrySet()) {
                    processClCount++;
                    String renameCL = clEntry.getKey();
                    String sourceCl = clEntry.getValue();
                    SdbCLFullInfo renameClFullInfo = new SdbCLFullInfo(renameCL);
                    SdbCLFullInfo sourceClFullInfo = new SdbCLFullInfo(sourceCl);
                    if (!renameClFullInfo.getCsName().equals(sourceClFullInfo.getCsName())) {
                        // 重命名的表和源表不是同个集合空间，不是正常情况，不删除
                        logger.warn(
                                "rename cl with source cl are not in the same cs, skip clean rename cl, site={}, sourceCL={}, renameCL={}",
                                site, sourceCl, renameCL);
                        continue;
                    }
                    if (!sdb.isCollectionSpaceExist(renameClFullInfo.getCsName())) {
                        continue;
                    }
                    CollectionSpace collectionSpace = sdb
                            .getCollectionSpace(renameClFullInfo.getCsName());
                    if (!collectionSpace.isCollectionExist(renameClFullInfo.getClName())) {
                        continue;
                    }
                    if (!collectionSpace.isCollectionExist(sourceClFullInfo.getClName())) {
                        // 如果重命名的表存在，但是源表不存在，这种情况也不删
                        logger.warn(
                                "rename cl exist, but source cl are not exist, skip clean rename cl, site={}, sourceCL={}, renameCL={}",
                                site, sourceCl, renameCL);
                        saveCleanProgress(site, sourceCl, renameCL, null, false,
                                "rename cl exist, but source cl are not exist");
                        continue;
                    }

                    logger.info(
                            "drop renamed cl, because the cl already map to datasource, and renamed cl has exceeded the max retention days, site={}, sourceCL={}, renameCL={}",
                            site, sourceCl, renameCL);
                    try {
                        collectionSpace.dropCollection(renameClFullInfo.getClName());
                        saveCleanProgress(site, sourceCl, renameCL, System.currentTimeMillis(),
                                true, null);
                    }
                    catch (BaseException e) {
                        if (e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                            logger.warn(
                                    "clean rename cl failed, site={}, sourceCL={}, renameCL={}",
                                    site, sourceCl, renameCL, e);
                            saveCleanProgress(site, sourceCl, renameCL, null, false,
                                    e.getMessage());
                        }
                    }
                    if (checkTimeout() || !running) {
                        return;
                    }
                }
            }
            finally {
                if (sdb != null) {
                    sourceDataService.releaseConnection(sdb);
                }
            }
        }
    }

    private void saveCleanProgress(String site, String sourceCL, String renameCl, Long cleanTime,
            boolean success, String detail) {
        try {
            ScheduleServer.getInstance().saveTaskCleanProgress(getTaskId(), site, sourceCL,
                    renameCl, cleanTime, success, detail);
        }
        catch (Exception e) {
            logger.warn(
                    "save data switch progress failed, taskId={}, site={}, sourceCL={}, renameCL={}, cleanTime={}",
                    getTaskId(), site, sourceCL, renameCl, cleanTime);
        }

    }

    private void savePlan(Map<String, Map<String, String>> needCleanRenameCl) throws Exception {
        BSONObject plan = new BasicBSONObject();
        needCleanRenameCl.forEach((key, value) -> {
            plan.put(key, value.keySet());
        });
        try {
            TaskUpdator updator = new TaskPlanUpdator(taskId, plan);
            updator.doUpdate();
        }
        catch (Exception e) {
            logger.error("update task plan failed:taskId=" + getTaskId(), e);
            throw e;
        }
    }

    private Map<String, Map<String, String>> getNeedCleanRenameCl() throws Exception {
        // key: site value: renameCl->sourceCL
        Map<String, Map<String, String>> siteClMap = new HashMap<>();
        for (Object o : cleanRange) {
            BSONObject range = (BSONObject) o;
            String cleanSite = BsonUtils.getString(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CLEAN_SITE);
            String cleanSiteRegex = BsonUtils.getString(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CLEAN_SITE_REGEX);
            List<String> clList = BsonUtils.getStringArray(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CL_LIST);
            List<String> csRegex = BsonUtils.getStringArray(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CS_REGEX);
            List<String> csList = BsonUtils.getStringArray(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CS_LIST);
            List<String> clRegex = BsonUtils.getStringArray(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CL_REGEX);
            Integer maxRetentionDays = BsonUtils.getIntegerOrElse(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_MAX_RETENTIONS_DAYS, 7);
            long maxRetentionTime = maxRetentionDays * 24 * 60 * 60 * 1000L;
            if (cleanSite != null && !cleanSite.isEmpty()) {
                SiteInfo site = ScheduleServer.getInstance().getSite(cleanSite);
                if (site == null) {
                    throw new ScheduleServerException(ScheduleServerError.RECORD_NOT_EXISTS,
                            "clean site not exist:" + cleanSite);
                }
                Map<String, String> map = siteClMap.computeIfAbsent(cleanSite,
                        k -> new HashMap<>());

                MetaCursor cursor = null;
                try {
                    BSONObject matcher = new BasicBSONObject();
                    matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_STATUS,
                            ScheduleDefine.CollectionDataSwitchStatus.DATA_SWITCHED);
                    matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_SITE_NAME,
                            cleanSite);
                    cursor = ScheduleServer.getInstance().queryCollectionDataSwitchEvent(matcher);
                    while (cursor.hasNext()) {
                        BSONObject event = cursor.getNext();
                        String clFullName = BsonUtils.getStringChecked(event,
                                FieldName.CollectionDataSwitchEvent.FIELD_COLLECTION_NAME);
                        Long dataSwitchTime = BsonUtils.getLongChecked(event,
                                FieldName.CollectionDataSwitchEvent.FIELD_DATA_SWITCHED_TIME);
                        String renameCL = BsonUtils.getStringChecked(event,
                                FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_CL_RENAME);
                        if (map.containsKey(renameCL)) {
                            continue;
                        }
                        if (isCanClean(clFullName, dataSwitchTime, maxRetentionTime, clList,
                                clRegex, csRegex, csList)) {
                            map.put(renameCL, clFullName);
                        }
                    }
                }
                finally {
                    if (cursor != null) {
                        cursor.close();
                    }
                }
            }
            else {
                MetaCursor cursor = null;
                try {
                    BSONObject matcher = new BasicBSONObject();
                    matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_STATUS,
                            ScheduleDefine.CollectionDataSwitchStatus.DATA_SWITCHED);
                    matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_SITE_NAME,
                            new BasicBSONObject("$regex", cleanSiteRegex));
                    cursor = ScheduleServer.getInstance().queryCollectionDataSwitchEvent(matcher);
                    while (cursor.hasNext()) {
                        BSONObject event = cursor.getNext();
                        String sourceSite = BsonUtils.getStringChecked(event,
                                FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_SITE_NAME);
                        String clFullName = BsonUtils.getStringChecked(event,
                                FieldName.CollectionDataSwitchEvent.FIELD_COLLECTION_NAME);
                        Long dataSwitchTime = BsonUtils.getLongChecked(event,
                                FieldName.CollectionDataSwitchEvent.FIELD_DATA_SWITCHED_TIME);
                        String renameCL = BsonUtils.getStringChecked(event,
                                FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_CL_RENAME);
                        Map<String, String> map = siteClMap.computeIfAbsent(sourceSite,
                                k -> new HashMap<>());
                        if (map.containsKey(renameCL)) {
                            continue;
                        }
                        if (isCanClean(clFullName, dataSwitchTime, maxRetentionTime, clList,
                                clRegex, csRegex, csList)) {
                            map.put(renameCL, clFullName);
                        }
                    }
                }
                finally {
                    if (cursor != null) {
                        cursor.close();
                    }
                }
            }
        }

        // key: site value: renameCl->sourceCL
        Map<String, Map<String, String>> finalSiteClMap = new HashMap<>();
        for (Map.Entry<String, Map<String, String>> entry : siteClMap.entrySet()) {
            String site = entry.getKey();
            Map<String, String> map = finalSiteClMap.computeIfAbsent(site, k -> new HashMap<>());
            DataServiceWrapper dataService = ScheduleServer.getInstance()
                    .getDataServiceWrapper(site);
            Sequoiadb sdb = null;
            try {
                sdb = dataService.getConnection();

                try (DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG,
                        new BasicBSONObject("Name",
                                new BasicBSONObject("$in", entry.getValue().keySet())),
                        new BasicBSONObject("Name", 1), null)) {
                    while (cursor.hasNext()) {
                        BSONObject next = cursor.getNext();
                        String name = BsonUtils.getStringChecked(next, "Name");
                        map.put(name, entry.getValue().get(name));
                    }
                }
            }
            finally {
                if (sdb != null) {
                    dataService.releaseConnection(sdb);
                }
            }
        }
        return finalSiteClMap;
    }

    private boolean isCanClean(String cllFullName, Long dataSwitchTime, long maxRetentionTime,
            List<String> clList, List<String> clRegex, List<String> csRegex, List<String> csList) {
        if (System.currentTimeMillis() - dataSwitchTime < maxRetentionTime) {
            // 没到可以删除的时间
            return false;
        }

        if (!CollectionUtils.isEmpty(clList)) {
            if (clList.contains(cllFullName)) {
                return true;
            }
        }

        if (!CollectionUtils.isEmpty(clRegex)) {
            for (String regex : clRegex) {
                if (cllFullName.matches(regex)) {
                    return true;
                }
            }
        }

        String csName = new SdbCLFullInfo(cllFullName).getCsName();
        if (!CollectionUtils.isEmpty(csList)) {
            if (csList.contains(csName)) {
                return true;
            }
        }

        if (!CollectionUtils.isEmpty(csRegex)) {
            for (String regex : csRegex) {
                if (csName.matches(regex)) {
                    return true;
                }
            }
        }

        return false;
    }

    private boolean checkTimeout() {
        if (isTimeout) {
            return true;
        }
        if (maxExecTime > 0 && System.currentTimeMillis() - taskStartTime > maxExecTime) {
            this.isTimeout = true;
        }
        return isTimeout;
    }

    @Override
    public String getTaskId() {
        return taskId;
    }

    @Override
    public int getTaskType() {
        return ScheduleDefine.TaskType.CLEAN;
    }

    @Override
    public TaskEntity getTaskInfo() {
        return taskEntity;
    }

    @Override
    public void unlockTransactionLock() {
        if (transactionLock != null) {
            transactionLock.unlock();
            transactionLock = null;
        }
    }
}
