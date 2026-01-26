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

   Source File Name = TaskServiceImpl.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.service.impl;

import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.core.ScheduleServer;
import com.sequoiadb.schedule.core.task.ScheduleTaskBase;
import com.sequoiadb.schedule.core.task.ScheduleTaskMgr;
import com.sequoiadb.schedule.dao.TaskDao;
import com.sequoiadb.schedule.dao.TaskProgressDao;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleSystemException;
import com.sequoiadb.schedule.lock.TransactionLock;
import com.sequoiadb.schedule.lock.TransactionLockFactory;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.model.ServerNodeEntity;
import com.sequoiadb.schedule.model.TaskEntity;
import com.sequoiadb.schedule.service.TaskService;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.List;

@Service
public class TaskServiceImpl implements TaskService {
    private static final Logger logger = LoggerFactory.getLogger(TaskServiceImpl.class);

    @Autowired
    private TaskDao taskDao;

    @Autowired
    private TaskProgressDao taskProgressDao;

    @Autowired
    private TransactionLockFactory transactionLockFactory;

    @Override
    public long getTaskCount(BSONObject filter) throws Exception {
        return taskDao.countTask(filter);
    }

    @Override
    public List<BSONObject> listTasks(BSONObject condition, BSONObject orderby, long skip,
            long limit) throws Exception {
        List<BSONObject> result = new ArrayList<>();
        MetaCursor cursor = null;
        try {
            cursor = taskDao.listTask(condition, orderby, skip, limit);
            while (cursor.hasNext()) {
                BSONObject obj = cursor.getNext();
                result.add(obj);
            }
            return result;
        }
        finally {
            if (null != cursor) {
                cursor.close();
            }
        }
    }

    @Override
    public void notifyTask(String taskId, int notifyType) throws ScheduleServerException {
        if (notifyType == ScheduleDefine.TaskNotifyType.TASK_CREATE) {
            startTask(taskId);
        }
        else if (notifyType == ScheduleDefine.TaskNotifyType.TASK_STOP) {
            // stop task
            innerStopTask(taskId);
        }
        else {
            throw new IllegalArgumentException("unrecognized notify type:type=" + notifyType);
        }
    }

    @Override
    public List<BSONObject> listTaskProgress(String taskId) throws Exception {
        List<BSONObject> result = new ArrayList<>();
        MetaCursor cursor = null;
        try {
            cursor = taskProgressDao.listTaskProgress(taskId);
            while (cursor.hasNext()) {
                BSONObject obj = cursor.getNext();
                result.add(obj);
            }
            return result;
        }
        finally {
            if (null != cursor) {
                cursor.close();
            }
        }
    }

    @Override
    public void stopTask(String taskId) throws Exception {
        BSONObject matcher = new BasicBSONObject(FieldName.Task.FIELD_ID, taskId);
        TaskEntity taskEntity = null;
        try {
            taskEntity = taskDao.queryOne(matcher, null);
        }
        catch (Exception e) {
            throw new ScheduleSystemException("stop task failed, query task failed:taskId=" + taskId + ",error=" + e, e);
        }

        int runningFlag = taskEntity.getRunningFlag();
        if (runningFlag != ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_INIT
                && runningFlag != ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_RUNNING
                && runningFlag != ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_CANCEL) {
            // have been finished
            logger.warn("ignore stop task:taskId=" + taskId + ",runningFlag=" + runningFlag);
            return;
        }

        startCancelTask(taskId);

        ServerNodeEntity runServer = ScheduleServer.getInstance().getServer(taskEntity.getRunServer());
        if (isLocalServer(runServer)) {
            innerStopTask(taskId);
        }
        else {
            notifyStop(runServer, taskId);
        }
        logger.info("stop task:taskId=" + taskId);
    }

    private void notifyStop(ServerNodeEntity server, String taskId) {
        String targetUrl = server.getHostName() + ":" + server.getPort();
        ScheduleServer.getInstance().notifyTask(targetUrl, taskId, 2);
    }

    private void startCancelTask(String taskId) throws ScheduleSystemException {
        try {
            BSONObject matcher = new BasicBSONObject(FieldName.Task.FIELD_ID, taskId);
            BSONObject flagList = new BasicBSONList();
            flagList.put("0", ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_INIT);
            flagList.put("1", ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_RUNNING);
            BSONObject inFlag = new BasicBSONObject("$in", flagList);
            matcher.put(FieldName.Task.FIELD_RUNNING_FLAG, inFlag);

            BSONObject newValue = new BasicBSONObject(FieldName.Task.FIELD_RUNNING_FLAG,
                    ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_CANCEL);
            newValue.put(FieldName.Task.FIELD_DETAIL, "user cancel");
            BSONObject updator = new BasicBSONObject("$set", newValue);

            taskDao.update(matcher, updator);
        }
        catch (Exception e) {
            throw new ScheduleSystemException("cancel task failed,taskId=" + taskId, e);
        }
    }



    private void innerStopTask(String taskId){
        ScheduleTaskBase task = null;
        try {
            task = ScheduleTaskMgr.getInstance().getTask(taskId);
            if (null == task) {
                logger.warn("task have been stopped:taskId=" + taskId);
                return;
            }

            task.stop();
            logger.info("task stop by user:taskId=" + taskId);
        }
        catch (Exception e) {
            logger.warn("stop task failed", e);
        }
    }

    private boolean isLocalServer(ServerNodeEntity server) {
        return ScheduleServer.getInstance().isLocalServer(server);
    }

    private void startTask(String taskId) throws ScheduleServerException {
        BSONObject matcher = new BasicBSONObject(FieldName.Task.FIELD_ID, taskId);
        TaskEntity taskEntity = null;
        try {
            taskEntity = taskDao.queryOne(matcher, null);
        }
        catch (Exception e) {
            throw new ScheduleSystemException("start task failed, query task failed:taskId=" + taskId + ",error=" + e, e);
        }

        if (null == taskEntity) {
            throw new ScheduleServerException(ScheduleServerError.RECORD_NOT_EXISTS,
                    "start task failed, task is not exist:taskId=" + taskId);
        }

        ScheduleTaskBase task = null;
        boolean lockTransferred = false;
        TransactionLock transactionLock = transactionLockFactory
                .create(ScheduleDefine.TransactionLockKey.LOCK_TASK, taskId);
        try {
            transactionLock.lock();
            taskEntity = taskDao.queryOne(matcher, null);
            if (null == taskEntity) {
                logger.debug("task not found after lock, skip start: taskId={}", taskId);
                return;
            }

            int runningFlag = taskEntity.getRunningFlag();
            if (runningFlag != ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_INIT) {
                logger.debug("task already running or finished, skip start: taskId={}, flag={}",
                        taskId, runningFlag);
                return;
            }

            ScheduleTaskBase existTask = ScheduleTaskMgr.getInstance().getTask(taskId);
            if (null != existTask) {
                logger.debug("task already exists in memory, skip start: taskId={}", taskId);
                return;
            }

            // 2. create task instance
            task = ScheduleTaskMgr.getInstance().createTask(taskEntity, transactionLock);
            lockTransferred = true;
        }
        catch (ScheduleServerException e) {
            if (e.getError() == ScheduleServerError.TRANSACTION_LOCK_TIMEOUT) {
                return;
            }
            throw new ScheduleServerException(e.getError(),
                    "create task failed:taskId=" + taskId + ",error=" + e, e);
        }
        catch (Exception e) {
            throw new ScheduleSystemException("create task failed:taskId=" + taskId + ",error=" + e,
                    e);
        }
        finally {
            if (!lockTransferred) {
                transactionLock.unlock();
            }
        }

        // 4. start task background
        try {
            task.start();
            logger.info("task started: taskId={}", taskId);
        }
        catch (Exception e) {
            logger.error("start task failed:taskId={}", taskId, e);
        }
    }
}
