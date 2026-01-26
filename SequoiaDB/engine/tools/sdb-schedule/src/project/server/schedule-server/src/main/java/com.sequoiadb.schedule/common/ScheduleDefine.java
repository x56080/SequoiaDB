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

   Source File Name = ScheduleDefine.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.common;

public class ScheduleDefine {
    public static class ScheduleType {
        public static final String TRANSFER = "transfer";
        public static final String DATA_SWITCH = "data_switch";
        public static final String CLEAN = "clean";
    }

    public static class TaskType {
        public static final int TRANSFER = 1;
        public static final int DATA_SWITCH = 2;
        public static final int CLEAN = 3;

    }

    public static class TaskRunningFlag {
        public static final int SCHEDULE_TASK_INIT = 1;
        public static final int SCHEDULE_TASK_RUNNING = 2;
        public static final int SCHEDULE_TASK_FINISH = 3;
        public static final int SCHEDULE_TASK_ABORT = 4;
        public static final int SCHEDULE_TASK_TIMEOUT = 5;
        public static final int SCHEDULE_TASK_CANCEL = 6;
    }

    public static class TaskNotifyType {
        private TaskNotifyType() {
            super();

        }

        public static final int TASK_CREATE = 1;
        public static final int TASK_STOP = 2;
    }

    public static class Job {
        public static final int JOB_TYPE_TASK = 100;
        public static final int JOB_TYPE_UPDATE_TASK_STATUS = 101;
    }

    public static class TransactionLockKey {
        public static final String LOCK_TASK = "task";
        public static final String LOCK_COLLECTION = "collection";
    }

    public static class CollectionDataSwitchStatus {
        public static final String DATA_SWITCHED = "data_switched";
        public static final String DATA_SWITCHING = "data_switching";
    }

    public static class GlobalConfKey {
        public static final String KEY_RENAME_CL_RETENTION_DAYS = "rename_cl_retention_days";
        public static final String KEY_TASK_RECORD_RETENTION_DAYS = "task_record_retention_days";

    }
}
