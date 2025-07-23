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

   Source File Name = TaskTable.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.core;

public class TaskTable {
    public static final String TASK_TYPE  = "TaskType";
    public static final String TASK_ID    = "TaskId";

    public static final String TASK_INDEX = "taskIndex";

    public static final int TASK_TYPE_DELIMITER     = 1;
    public static final int TASK_COMPLETE_UPLOAD    = 2;

    private int taskType;
    private long taskId;

    public void setTaskId(long taskId) {
        this.taskId = taskId;
    }

    public long getTaskId() {
        return taskId;
    }

    public void setTaskType(int taskType) {
        this.taskType = taskType;
    }

    public int getTaskType() {
        return taskType;
    }
}
