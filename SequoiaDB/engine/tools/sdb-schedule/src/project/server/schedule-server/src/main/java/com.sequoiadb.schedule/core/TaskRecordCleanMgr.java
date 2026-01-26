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

   Source File Name = TaskRecordCleanMgr.java

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

import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.common.TaskEntityTranslator;
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.core.elect.LeaderElect;
import com.sequoiadb.schedule.dao.GlobalConfDao;
import com.sequoiadb.schedule.dao.TaskDao;
import com.sequoiadb.schedule.dao.TaskProgressDao;
import com.sequoiadb.schedule.dao.TransactionFactory;
import com.sequoiadb.schedule.dao.TransactionLockDao;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.metasource.template.ITransaction;
import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;
import java.util.ArrayList;
import java.util.List;

@Component
public class TaskRecordCleanMgr {
    private static final Logger logger = LoggerFactory.getLogger(TaskRecordCleanMgr.class);
    private static final int CLEAN_INTERVAL_MS = 60 * 60 * 1000; // 1 hour
    @Autowired
    private LeaderElect leaderElect;

    @Autowired
    private TaskDao taskDao;

    @Autowired
    private TransactionLockDao transactionLockDao;

    @Autowired
    private TransactionFactory transactionFactory;

    @Autowired
    private TaskProgressDao taskProgressDao;

    @Autowired
    private GlobalConfDao globalConfDao;

    private static final long TASK_RECORD_RETENTIONS_MS = 30 * 24 * 60 * 60 * 1000L; // 30 days

    private Timer cleanTimer;

    @PostConstruct
    public void init() {
        if (cleanTimer == null) {
            cleanTimer = TimerFactory.createTimer("TaskRecordCleanTimer");
        }
        cleanTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    if (!leaderElect.isLeader()) {
                        return;
                    }
                    doClean();
                }
                catch (Exception e) {
                    logger.warn("do clean task records failed", e);
                }
            }
        }, 0, CLEAN_INTERVAL_MS);
    }

    private void doClean() throws Exception {
        String globalConfValue = globalConfDao.getGlobalConfValue(ScheduleDefine.GlobalConfKey.KEY_TASK_RECORD_RETENTION_DAYS);
        long maxRetentionTime = TASK_RECORD_RETENTIONS_MS;
        if (globalConfValue != null) {
            try {
                int retentionsDays = Integer.parseInt(globalConfValue);
                maxRetentionTime = retentionsDays * 24 * 60 * 60 * 1000L;
            }
            catch (NumberFormatException e) {
                logger.warn("parse task_record_retention_days failed, use default value: {}", TASK_RECORD_RETENTIONS_MS, e);
            }
        }
        logger.debug("current task record retention time is {} ms", maxRetentionTime);
        List<Integer> notFinishedStatus = new ArrayList<>();
        notFinishedStatus.add(ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_INIT);
        notFinishedStatus.add(ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_RUNNING);
        BasicBSONList andList = new BasicBSONList();
        andList.add(new BasicBSONObject(FieldName.Task.FIELD_CREATE_TIME,
                new BasicBSONObject("$lte", System.currentTimeMillis() - maxRetentionTime)));
        andList.add(new BasicBSONObject(FieldName.Task.FIELD_RUNNING_FLAG,
                new BasicBSONObject("$nin", notFinishedStatus)));
        BSONObject matcher = new BasicBSONObject("$and", andList);
        MetaCursor metaCursor = null;
        try {
            metaCursor = taskDao.listTask(matcher, null, 0, -1);
            List<String> batchIds = new ArrayList<>();
            while (metaCursor.hasNext()) {
                BSONObject taskObj = metaCursor.getNext();
                TaskEntity taskEntity = TaskEntityTranslator.fromBSONObject(taskObj);
                batchIds.add(taskEntity.getId());
                if (batchIds.size() >= 100) {
                    batchDelete(batchIds);
                    batchIds.clear();
                }
            }
            if (batchIds.size() > 0) {
                batchDelete(batchIds);
                batchIds.clear();
            }
        }
        finally {
            if (metaCursor != null) {
                metaCursor.close();
            }
        }
    }

    private void batchDelete(List<String> batchIds) throws Exception {
        ITransaction transaction = transactionFactory.createTransaction();
        try {
            transaction.begin();
            taskDao.delete(new BasicBSONObject(FieldName.Task.FIELD_ID,
                    new BasicBSONObject("$in", batchIds)), transaction);
            taskProgressDao.delete(new BasicBSONObject(FieldName.TaskProgress.FIELD_TASK_ID,
                    new BasicBSONObject("$in", batchIds)), transaction);
            BSONObject lockMatcher = new BasicBSONObject();
            lockMatcher.put(FieldName.TransactionLock.FIELD_LOCK_KEY,
                    ScheduleDefine.TransactionLockKey.LOCK_TASK);
            lockMatcher.put(FieldName.TransactionLock.FIELD_LOCK_VALUE,
                    new BasicBSONObject("$in", batchIds));
            transactionLockDao.delete(lockMatcher, transaction);
            transaction.commit();
        }
        catch (Exception e) {
            transaction.rollback();
            throw e;
        }
    }

    @PreDestroy
    public void destroy() {
        if (cleanTimer != null) {
            cleanTimer.cancel();
            cleanTimer = null;
        }
    }
}
