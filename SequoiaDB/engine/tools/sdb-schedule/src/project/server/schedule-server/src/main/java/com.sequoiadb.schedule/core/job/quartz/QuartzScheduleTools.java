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

   Source File Name = QuartzScheduleTools.java

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
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.core.ScheduleServer;
import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.quartz.Job;
import org.quartz.JobBuilder;
import org.quartz.JobDataMap;
import org.quartz.JobDetail;
import org.quartz.JobKey;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

class QuartzScheduleTools {
    private static final Logger logger = LoggerFactory.getLogger(QuartzScheduleTools.class);

    public static JobKey createJobKey(String id) {
        return new JobKey(id);
    }

    public static JobDetail createJobDetail(Class<? extends Job> jobClass, JobDataMap dataMap,
            JobKey jobKey) {
        JobDetail detail = JobBuilder.newJob(jobClass).setJobData(dataMap).withDescription("")
                .withIdentity(jobKey).build();

        return detail;
    }

    public static TaskEntity createTask(int taskType, String taskId, BSONObject content,
            String runServer, String scheduleId, long createTime) {
        TaskEntity task = new TaskEntity();
        task.setId(taskId);
        task.setRunningFlag(ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_INIT);
        task.setType(taskType);
        task.setScheduleId(scheduleId);
        task.setCreateTime(createTime);
        task.setContent(content);
        task.setRunServer(runServer);
        return task;
    }

    public static void setTaskAbortSilence(String taskId, String msg) {
        try {
            BSONObject matcher = new BasicBSONObject(FieldName.Task.FIELD_ID, taskId);
            BSONObject newValue = new BasicBSONObject();
            newValue.put(FieldName.Task.FIELD_RUNNING_FLAG,
                    ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_ABORT);
            newValue.put(FieldName.Task.FIELD_DETAIL, msg);
            BSONObject modify = new BasicBSONObject("$set", newValue);
            ScheduleServer.getInstance().updateTask(matcher, modify);
        }
        catch (Exception e) {
            logger.warn(
                    "Failed to update task running flag to abort:taskId={}, runningFlag={}, detail={}",
                    taskId, ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_ABORT, msg, e);
        }
    }
}
