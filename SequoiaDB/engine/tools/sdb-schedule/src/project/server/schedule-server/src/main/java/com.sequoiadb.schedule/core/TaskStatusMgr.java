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

   Source File Name = TaskStatusMgr.java

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
import com.sequoiadb.schedule.dao.TaskDao;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.lock.TransactionLock;
import com.sequoiadb.schedule.lock.TransactionLockFactory;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

public class TaskStatusMgr {
    private static final Logger logger = LoggerFactory.getLogger(TaskStatusMgr.class);
    private static final TaskStatusMgr instance = new TaskStatusMgr();
    private TaskDao taskDao;
    private TransactionLockFactory transactionLockFactory;

    private Timer checkTimer;
    private static final int CHECK_INTERVAL = 60 * 1000; // 1 minutes
    private static final int TASK_MAX_RUNNING_TIME = 10 * 60 * 1000; // 10 minutes

    public static TaskStatusMgr getInstance() {
        return instance;
    }

    private TaskStatusMgr() {
    }

    public void init(TaskDao taskDao, TransactionLockFactory transactionLockFactory) {
        this.taskDao = taskDao;
        this.transactionLockFactory = transactionLockFactory;
    }

    public void start() {
        if (checkTimer != null) {
            checkTimer.cancel();
            checkTimer = null;
        }

        checkTimer = TimerFactory.createTimer("TaskStatusCheckTimer");
        checkTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    checkTaskStatus();
                }
                catch (Exception e) {
                    logger.warn("failed to check task status", e);
                }
            }
        }, 10000, CHECK_INTERVAL);
    }

    private void checkTaskStatus() throws Exception {
        List<Integer> notFinishedStatus = new ArrayList<>();
        notFinishedStatus.add(ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_INIT);
        notFinishedStatus.add(ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_RUNNING);
        BasicBSONList andList = new BasicBSONList();
        andList.add(new BasicBSONObject(FieldName.Task.FIELD_CREATE_TIME,
                new BasicBSONObject("$lte", System.currentTimeMillis() - TASK_MAX_RUNNING_TIME)));
        andList.add(new BasicBSONObject(FieldName.Task.FIELD_RUNNING_FLAG,
                new BasicBSONObject("$in", notFinishedStatus)));
        BSONObject matcher = new BasicBSONObject("$and", andList);
        MetaCursor metaCursor = null;
        try {
            metaCursor = taskDao.listTask(matcher, null, 0, -1);
            while (metaCursor.hasNext()) {
                BSONObject taskObj = metaCursor.getNext();
                TaskEntity taskEntity = TaskEntityTranslator.fromBSONObject(taskObj);
                checkTask(taskEntity);
            }
        }
        finally {
            if (metaCursor != null) {
                metaCursor.close();
            }
        }
    }

    private void checkTask(TaskEntity taskEntity) {
        TransactionLock transactionLock = transactionLockFactory
                .create(ScheduleDefine.TransactionLockKey.LOCK_TASK, taskEntity.getId());
        boolean needNotify = false;
        try {
            transactionLock.lock();
            TaskEntity task = taskDao.queryById(taskEntity.getId());
            int runningFlag = task.getRunningFlag();
            if (runningFlag == ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_INIT) {
                needNotify = true;
            }
            else if (runningFlag == ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_RUNNING) {
                // 任务执行中，但是能获取到锁，说明任务执行的服务器down机了，设置任务为失败
                logger.warn("the task running_flag is running, but run server may be down, set task to failed: taskId={}", task.getId());
                setTaskRunFailed(task.getId(),
                        "run server may be down, task set to failed by leader server");
            }
            else {
                logger.debug("task already finished, no need to check, taskId=" + task.getId());
            }
        }
        catch (ScheduleServerException e) {
            if (e.getError() != ScheduleServerError.TRANSACTION_LOCK_TIMEOUT) {
                logger.warn("failed to check task, taskId=" + taskEntity.getId(), e);
            }
            else {
                logger.debug("lock failed, the task may be running on server, skip check, taskId="
                        + taskEntity.getId(), e);
            }
        }
        catch (Exception e) {
            logger.warn("failed to check task, taskId=" + taskEntity.getId(), e);
        }
        finally {
            transactionLock.unlock();
        }

        if (needNotify) {
            logger.debug("task may not be nottified to run, notify task again: task={}", taskEntity);
            notifyTask(taskEntity);
        }
    }

    private void notifyTask(TaskEntity task) {
        try {
            ScheduleServer.getInstance().notifyTask(task.getRunServer(), task.getId(), 1);
        }
        catch (Exception e) {
            logger.error("notify task failed, alter task running flag to abort: task={}", task, e);
            setTaskRunFailed(task.getId(),
                    "retry notify task to run server failed, " + e.toString());
        }
    }

    private void setTaskRunFailed(String taskId, String errorDetail) {
        BSONObject newValue = new BasicBSONObject();
        newValue.put(FieldName.Task.FIELD_RUNNING_FLAG,
                ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_ABORT);
        newValue.put(FieldName.Task.FIELD_STOP_TIME, System.currentTimeMillis());
        newValue.put(FieldName.Task.FIELD_DETAIL, errorDetail);
        try {
            taskDao.updateByTaskId(taskId, newValue);
        }
        catch (Exception e) {
            logger.error(
                    "set task run failed failed:taskId=" + taskId + ", updateValue=" + newValue, e);
        }
    }

    public void clear() {
        if (checkTimer != null) {
            checkTimer.cancel();
            checkTimer = null;
        }
    }
}
