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

   Source File Name = TaskRunningUpdator.java

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
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class TaskRunningUpdator implements TaskUpdator {

    private String taskId;
    private long startTime;

    public TaskRunningUpdator(String taskId, long startTime) {
        this.taskId = taskId;
        this.startTime = startTime;
    }

    @Override
    public String getTaskId() {
        return taskId;
    }

    @Override
    public void doUpdate() throws Exception {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.Task.FIELD_ID, taskId);
        matcher.put(FieldName.Task.FIELD_RUNNING_FLAG,
                ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_INIT);
        BSONObject newValue = new BasicBSONObject();
        newValue.put(FieldName.Task.FIELD_RUNNING_FLAG,
                ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_RUNNING);
        newValue.put(FieldName.Task.FIELD_START_TIME, startTime);
        BSONObject modifier = new BasicBSONObject();
        modifier.put("$set", newValue);
        ScheduleServer.getInstance().updateTask(matcher, modifier);
    }
}
