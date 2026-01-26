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

   Source File Name = ScheduleTaskMgr.java

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

import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.lock.TransactionLock;
import com.sequoiadb.schedule.model.TaskEntity;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.locks.ReentrantLock;

@Component
public class ScheduleTaskMgr {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleTaskMgr.class);
    private static volatile ScheduleTaskMgr taskManager;

    private Map<String, ScheduleTaskBase> taskMap = new HashMap<>();
    private ReentrantLock lock = new ReentrantLock();
    private ScheduleJobUpdateTask updateTaskStatusJob = new ScheduleJobUpdateTask();

    public ScheduleTaskMgr(ScheduleJobManager scheduleJobManager) throws ScheduleServerException {
        try {
            scheduleJobManager.schedule(updateTaskStatusJob, 0);
            ScheduleTaskMgr.taskManager = this;
        }
        catch (Exception e) {
            logger.error("schedule update task job failed", e);
            throw e;
        }
    }

    public static ScheduleTaskMgr getInstance() {
        checkState();
        return taskManager;
    }

    private static void checkState() {
        if (taskManager == null) {
            throw new RuntimeException("ScheduleTaskMgr is not initialized");
        }
    }

    public void addTask(ScheduleTaskBase task) {
        checkState();
        lock.lock();
        try {
            taskMap.put(task.getTaskId(), task);
        }
        finally {
            lock.unlock();
        }
    }

    public ScheduleTaskBase getTask(String taskId) {
        checkState();
        lock.lock();
        try {
            return taskMap.get(taskId);
        }
        finally {
            lock.unlock();
        }
    }

    public ScheduleTaskBase createTask(TaskEntity taskEntity, TransactionLock transactionLock) {
        checkState();
        ScheduleTaskBase task = null;
        int type = taskEntity.getType();
        if (type == ScheduleDefine.TaskType.TRANSFER) {
            task = new SdbTransferTask(this, taskEntity, transactionLock);
        }
        else if (type == ScheduleDefine.TaskType.DATA_SWITCH) {
            task = new SdbDataSwitchTask(this, taskEntity, transactionLock);
        }
        else if (type == ScheduleDefine.TaskType.CLEAN) {
            task = new SdbCleanTask(this, taskEntity, transactionLock);
        }
        else {
            throw new RuntimeException("unrecognized task type:type=" + type);
        }
        return task;
    }

    public void addAsyncTaskUpdator(TaskUpdator updator) {
        checkState();
        updateTaskStatusJob.addUpdator(updator);
    }

    public void delTask(String taskId) {
        checkState();
        lock.lock();
        try {
            taskMap.remove(taskId);
        }
        finally {
            lock.unlock();
        }
    }
}
