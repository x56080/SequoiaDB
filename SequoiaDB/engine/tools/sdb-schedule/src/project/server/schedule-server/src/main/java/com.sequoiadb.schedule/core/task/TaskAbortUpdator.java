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

   Source File Name = TaskAbortUpdator.java

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
import com.sequoiadb.schedule.core.ScheduleServer;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class TaskAbortUpdator implements TaskUpdator {

    private String taskId;
    private String detail;
    private int runningFlag;
    private long processClCount;

    public TaskAbortUpdator(String taskId, int runningFlag, String detail, long processClCount) {
        this.taskId = taskId;
        this.detail = detail;
        this.runningFlag = runningFlag;
        this.processClCount = processClCount;
    }

    @Override
    public String getTaskId() {
        return taskId;
    }

    @Override
    public void doUpdate() throws Exception {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.Task.FIELD_ID, taskId);

        BSONObject newValue = new BasicBSONObject();
        newValue.put(FieldName.Task.FIELD_RUNNING_FLAG, runningFlag);
        newValue.put(FieldName.Task.FIELD_DETAIL, detail);
        newValue.put(FieldName.Task.FIELD_STOP_TIME, System.currentTimeMillis());
        newValue.put(FieldName.Task.FIELD_PROCESS_CL_count, processClCount);
        BSONObject modifier = new BasicBSONObject();
        modifier.put("$set", newValue);

        ScheduleServer.getInstance().updateTask(matcher, modifier);
    }
}
