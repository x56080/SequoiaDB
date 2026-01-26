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

   Source File Name = QuartzSdbScheduleJob.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core.job.quartz;

import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.LockWrapper;
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.core.ScheduleServer;
import com.sequoiadb.schedule.core.job.ScheduleJobInfo;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.model.ServerNodeEntity;
import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.quartz.JobExecutionContext;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class QuartzSdbScheduleJob extends QuartzScheduleJob {
    private static final Logger logger = LoggerFactory.getLogger(QuartzSdbScheduleJob.class);

    @Override
    public void execute(ScheduleJobInfo info, JobExecutionContext context)
            throws ScheduleServerException {
        logger.debug("schedule trigger: {}", info);

        // 检查本调度一个最新的Task（状态为 Running or Init），通过 notify 请求确认任务执行节点的运行状态，
        // 若正在运行则忽略本次调度的触发，否则正常触发新建一个 Task
        LockWrapper writeLock = ScheduleServer.getInstance().getWriteLock(info.getId());
        ServerNodeEntity runTaskServer;
        TaskEntity task;
        try {
            writeLock.lock();
            TaskEntity initOrRunningTask = getScheduleInitOrRunningTask(info.getId());
            if (initOrRunningTask != null) {
                if (initOrRunningTask.getRunningFlag() == ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_RUNNING) {
                    logger.info(
                            "the previous task is running, ignore this trigger: runningTask: {}",
                            initOrRunningTask);
                    return;
                }
                ServerNodeEntity server = ScheduleServer.getInstance()
                        .getServer(initOrRunningTask.getRunServer());
                if (server != null) {
                    try {
                        notifyTask(server, initOrRunningTask.getId());
                        logger.info(
                                "the previous task is running, ignore this trigger: runningTask: {}",
                                initOrRunningTask);
                        return;
                    }
                    catch (Exception e) {
                        logger.warn(
                                "failed to check the exist task status, ignore this trigger: oldTask={}",
                                initOrRunningTask, e);
                        return;
                    }
                }
                else {
                    logger.warn(
                            "schedule server not found, assume the previous task is not running: {}",
                            initOrRunningTask);
                }
            }

            runTaskServer = getRunTaskServer();
            if (null == runTaskServer) {
                throw new ScheduleServerException(ScheduleServerError.RECORD_NOT_EXISTS,
                        "no active schedule server to run task:" + info);
            }

            task = createTaskEntity(runTaskServer, info);

            try {
                ScheduleServer.getInstance().insertTask(task);
                logger.debug("task created: {}", task);
            }
            catch (Exception e) {
                throw new ScheduleServerException(ScheduleServerError.INTERNAL_ERROR,
                        "insert task failed:task=" + task, e);
            }

            try {
                notifyTask(runTaskServer, task.getId());
                logger.info("task has successfully started on server, taskId={}, server={}",
                        task.getId(), runTaskServer);
            }
            catch (Exception e) {
                if (isLocalServer(runTaskServer)) {
                    logger.error(
                            "notify task in local server failed, alter task running flag to abort: task={}",
                            task, e);
                    QuartzScheduleTools.setTaskAbortSilence(task.getId(), e.toString());
                }
                else {
                    logger.warn("notify task failed to server={}, retry run in local: task={} ",
                            runTaskServer, task, e);
                    runInLocal(task.getId());
                }
            }
        }
        finally {
            writeLock.unlock();
        }
    }

    protected abstract TaskEntity createTaskEntity(ServerNodeEntity runTaskServer,
            ScheduleJobInfo info) throws ScheduleServerException;

    private ServerNodeEntity getRunTaskServer() {
        return ScheduleServer.getInstance().getActiveServer();
    }

    private boolean isLocalServer(ServerNodeEntity server) {
        return ScheduleServer.getInstance().isLocalServer(server);
    }

    private void runInLocal(String taskId) {
        try {
            ServerNodeEntity localServer = ScheduleServer.getInstance().getLocalServer();
            updateTaskRunServer(localServer, taskId);
            notifyTask(localServer, taskId);
            logger.info("task has successfully started on local server, taskId={}, server={}",
                    taskId, localServer);
        }
        catch (Exception e) {
            logger.error(
                    "notify task in local server failed, alter task running flag to abort: taskId={}",
                    taskId, e);
            QuartzScheduleTools.setTaskAbortSilence(taskId, e.toString());
        }

    }

    public void updateTaskRunServer(ServerNodeEntity server, String taskId) {
        String targetUrl = server.getUrl();
        try {
            ScheduleServer.getInstance().updateTaskRunServer(targetUrl, taskId);
        }
        catch (Exception e) {
            logger.error("update task run server failed:taskId=" + taskId + ",server=" + targetUrl,
                    e);
        }
    }

    private TaskEntity getScheduleInitOrRunningTask(String scheduleId)
            throws ScheduleServerException {
        BSONObject condition = new BasicBSONObject();
        condition.put(FieldName.Task.FIELD_SCHEDULE_ID, scheduleId);

        BasicBSONList orCondition = new BasicBSONList();
        orCondition.add(new BasicBSONObject(FieldName.Task.FIELD_RUNNING_FLAG,
                ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_RUNNING));
        orCondition.add(new BasicBSONObject(FieldName.Task.FIELD_RUNNING_FLAG,
                ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_INIT));

        condition.put("$or", orCondition);
        try {
            return ScheduleServer.getInstance().queryLatestTask(condition);
        }
        catch (Exception e) {
            throw new ScheduleServerException(ScheduleServerError.INTERNAL_ERROR,
                    "query task failed:scheduleId=" + scheduleId, e);
        }
    }

    private void notifyTask(ServerNodeEntity server, String taskId) {
        String targetUrl = server.getHostName() + ":" + server.getPort();
        ScheduleServer.getInstance().notifyTask(targetUrl, taskId, 1);
    }
}
