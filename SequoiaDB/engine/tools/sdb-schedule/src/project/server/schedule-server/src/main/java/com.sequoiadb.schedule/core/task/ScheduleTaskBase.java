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

   Source File Name = ScheduleTaskBase.java

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

import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.core.ScheduleServer;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleSystemException;
import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class ScheduleTaskBase extends ScheduleBackgroundJob {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleTaskBase.class);
    protected ScheduleTaskMgr mgr;

    public ScheduleTaskBase(ScheduleTaskMgr mgr) {
        this.mgr = mgr;
    }

    public final void start() throws ScheduleServerException {
        try {
            // 任务添加进等待队列，若队列满了，则添加失败
            ScheduleJobManager.getInstance().executeScheduleTask(this);
            mgr.addTask(this);
        }
        catch (ScheduleServerException e) {
            abortTaskAndAsyncRedo(getTaskId(), ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_ABORT,
                    "start job failed", 0);
            unlockTransactionLock();
            throw e;
        }
        catch (Exception e) {
            abortTaskAndAsyncRedo(getTaskId(), ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_ABORT,
                    "start job failed", 0);
            unlockTransactionLock();
            throw new ScheduleSystemException("start job failed: taskId=" + getTaskId(), e);
        }
    }

    public void abortTaskAndAsyncRedo(String taskId, int flag, String detail, long processClCount) {
        TaskUpdator updator = new TaskAbortUpdator(taskId, flag, detail, processClCount);
        try {
            updator.doUpdate();
        }
        catch (Exception e) {
            mgr.addAsyncTaskUpdator(updator);
            logger.warn("abort task failed:taskId=" + taskId, e);
        }
    }

    public void stopTaskAndAsyncRedo(String taskId, long processClCount) {
        TaskUpdator updator = new TaskCancelUpdator(taskId, processClCount);
        try {
            updator.doUpdate();
        }
        catch (Exception e) {
            mgr.addAsyncTaskUpdator(updator);
            logger.warn("update task failed:taskId=" + taskId, e);
        }
    }

    public void finishTaskAndAsyncRedo(String taskId, long processClCount) {
        TaskUpdator updator = new TaskFinishUpdator(taskId, processClCount);
        try {
            updator.doUpdate();
        }
        catch (Exception e) {
            mgr.addAsyncTaskUpdator(updator);
            logger.warn("update task failed:taskId=" + taskId, e);
        }
    }

    @Override
    public final void _run() {
        try {
            _runTask();
        }
        catch (Throwable e) {
            logger.error("run task failed:taskId=" + getTaskId(), e);
            abortTaskAndAsyncRedo(getTaskId(), ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_ABORT,
                    e.toString(), 0);
        }
        finally {
            mgr.delTask(getTaskId());
        }
    }

    public void updateTaskRunning(String taskId, long taskStartTime) throws Exception {
        try {
            TaskUpdator updator = new TaskRunningUpdator(taskId, taskStartTime);
            updator.doUpdate();
        }
        catch (Exception e) {
            logger.error("update task running flag failed:taskId=" + getTaskId(), e);
            throw e;
        }
    }

    public static int getTaskRunningFlag(String taskId) throws Exception {
        TaskEntity task = ScheduleServer.getInstance().getTask(taskId);
        if (null == task) {
            throw new ScheduleServerException(ScheduleServerError.RECORD_NOT_EXISTS,
                    "task is not exist:taskId=" + taskId);
        }

        return task.getRunningFlag();
    }

    public abstract void _stop();

    public final void stop() {
        _stop();
    }

    public abstract void _runTask();

    @Override
    public final int getType() {
        return ScheduleDefine.Job.JOB_TYPE_TASK;
    }

    @Override
    public long getPeriod() {
        return 0;
    }

    public abstract String getTaskId();

    public abstract int getTaskType();

    public abstract TaskEntity getTaskInfo();

    public abstract void unlockTransactionLock();
}
