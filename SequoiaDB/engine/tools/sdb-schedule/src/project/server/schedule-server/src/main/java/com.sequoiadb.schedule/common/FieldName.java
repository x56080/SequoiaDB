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

   Source File Name = FieldName.java

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

public class FieldName {

    public static class Schedule {
        public static final String FIELD_ID = "id";
        public static final String FIELD_NAME = "name";
        public static final String FIELD_DESC = "desc";
        // string
        public static final String FIELD_TYPE = "type";
        // bson
        public static final String FIELD_CONTENT = "content";
        // string
        public static final String FIELD_CRON = "cron";
        // long (ms)
        public static final String FIELD_CREATE_TIME = "create_time";
        public static final String FIELD_UPDATE_TIME = "update_time";
        // boolean
        public static final String FIELD_ENABLE = "enable";

        public static final String FIELD_MAX_EXEC_TIME = "max_exec_time";

        public static final String FIELD_SCH_INFO = "sch_info";
        public static final String FIELD_CONTENT_SOURCE_SITE = "source_site";
        public static final String FIELD_CONTENT_TARGET_SITE = "target_site";
        public static final String FIELD_CONTENT_CS_LIST = "cs_list";
        public static final String FIELD_CONTENT_CS_REGEX = "cs_regex";
        public static final String FIELD_CONTENT_CL_LIST = "cl_list";
        public static final String FIELD_CONTENT_CL_REGEX = "cl_regex";
        public static final String FIELD_CONTENT_NO_WRITE_TIME_THRESHOLD = "no_write_time_threshold";
        public static final String FIELD_CONTENT_DELETE_MORE_LOB_IN_TARGET = "delete_more_lob_in_target";
        public static final String FIELD_CONTENT_PARTITION_INTERRUPTION = "partition_interruption";
        public static final String FIELD_CONTENT_DATA_DOMAIN = "data_domain";
        public static final String FIELD_CONTENT_CL_CREATE_TIME_THRESHOLD = "cl_create_time_threshold";
        public static final String FIELD_CONTENT_CHECK_DATA_CONSISTENT = "check_data_consistent";

        public static final String FIELD_CONTENT_CLEAN_RANGE = "clean_range";
        public static final String FIELD_CONTENT_CLEAN_RANGE_CLEAN_SITE = "clean_site";
        public static final String FIELD_CONTENT_CLEAN_RANGE_CLEAN_SITE_REGEX = "clean_site_regex";
        public static final String FIELD_CONTENT_CLEAN_RANGE_CL_LIST = "cl_list";
        public static final String FIELD_CONTENT_CLEAN_RANGE_CL_REGEX = "cl_regex";
        public static final String FIELD_CONTENT_CLEAN_RANGE_CS_REGEX = "cs_regex";
        public static final String FIELD_CONTENT_CLEAN_RANGE_CS_LIST = "cs_list";
        public static final String FIELD_CONTENT_CLEAN_RANGE_MAX_RETENTIONS_DAYS= "max_retention_days";
    }

    public static final class ServerNode {
        public static final String FIELD_HOSTNAME = "host_name";
        public static final String FIELD_PORT = "port";
        public static final String FIELD_IPADDR = "ip_Addr";
        public static final String FIELD_STATUS = "status";
        public static final String FIELD_LAST_HEART_TIME = "last_heart_time";
        public static final String FIELD_LEASE_NUM = "lease_num";

    }

    public static final class ScheduleServerElection {
        public static final String SERVER_TYPE = "server_type";
        public static final String NODE_URL = "node_url";
        public static final String LEASE_NUM = "lease_num";
        public static final String UPDATE_TIME = "update_time";

    }

    public static class Task {
        public static final String FIELD_ID = "id";
        public static final String FIELD_TYPE = "type";
        public static final String FIELD_CONTENT = "content";
        public static final String FIELD_RUN_SERVER = "run_server";
        public static final String FIELD_RUNNING_FLAG = "running_flag";
        public static final String FIELD_DETAIL = "detail";
        public static final String FIELD_START_TIME = "start_time";
        public static final String FIELD_STOP_TIME = "stop_time";
        public static final String FIELD_PROCESS_CL_count = "process_cl_count";
        public static final String FIELD_CREATE_TIME = "create_time";
        public static final String FIELD_SCHEDULE_ID = "schedule_id";
        public static final String FIELD_PLAN = "plan";
        public static final String FIELD_PLAN_CS = "cs";
        public static final String FIELD_PLAN_CL = "cl";
    }

    public static class Site {
        public static final String NAME = "name";
        public static final String URLS = "urls";
        public static final String USER = "user";
        public static final String PASSWORD = "password";
        public static final String DATASOURCE = "datasource";
    }

    public static class TaskProgress {
        public static final String FIELD_TASK_ID = "task_id";
        public static final String FIELD_COLLECTION_NAME = "collection";
        public static final String FIELD_SUCCESS_RECORD_NUM = "success_record_num";
        public static final String FIELD_FAILED_RECORD_NUM = "failed_record_num";
        public static final String FIELD_SUCCESS_LOB_NUM = "success_lob_num";
        public static final String FIELD_FAILED_LOB_NUM = "failed_lob_num";
        public static final String FIELD_TRANSFER_RECORD_SIZE = "transfer_record_size";
        public static final String FIELD_TRANSFER_LOB_SIZE = "transfer_lob_size";
        public static final String FIELD_CAN_DATA_SWITCH = "can_data_switch";
        public static final String FIELD_DATA_SWITCHED = "data_switched";

        public static final String FIELD_CLEAN_SITE = "clean_site";
        public static final String FIELD_SOURCE_CL = "source_cl";
        public static final String FIELD_RENAME_CL = "rename_cl";
        public static final String FIELD_CLEAN_TIME = "clean_time";
        public static final String FIELD_CLEAN_SUCCESS = "success";
        public static final String FIELD_CLEAN_DETAIL = "detail";

    }

    public static class TransactionLock {
        public static final String FIELD_LOCK_KEY = "lock_key";
        public static final String FIELD_LOCK_VALUE = "lock_value";
    }

    public static class CollectionSnapshotRecord {
        public static final String FIELD_COLLECTION_NAME = "collection";
        public static final String FIELD_SNAPSHOTS = "snapshots";
        public static final String FIELD_SITE_NAME = "site";
        public static final String FIELD_LAST_RECORD_TIME = "last_record_time";
        public static final String FIELD_RECORD_SNAPSHOT_EFFECTIVE = "record_snapshot_effective";
        public static final String FIELD_LOB_SNAPSHOT_EFFECTIVE = "lob_snapshot_effective";
    }

    public static class CollectionDataSwitchEvent {
        public static final String FIELD_COLLECTION_NAME = "collection";
        public static final String FIELD_SOURCE_SITE_NAME = "source_site";
        public static final String FIELD_TARGET_SITE_NAME = "target_site";
        public static final String FIELD_TARGET_DATASOURCE_NAME = "target_datasource";
        public static final String FIELD_SOURCE_CL_CATA_INFO = "source_cl_cata_info";
        public static final String FIELD_SOURCE_CL_ATTACH_INFO = "source_cl_attach_info";
        public static final String FIELD_SOURCE_CL_RENAME = "source_cl_rename";
        public static final String FIELD_STATUS = "status";
        public static final String FIELD_DATA_SWITCHED_TIME = "data_switch_time";
    }

    public static class CollectionTransferRecordStatus {
        public static final String FIELD_COLLECTION_NAME = "collection";
        public static final String FIELD_SOURCE_SITE_NAME = "source_site";
        public static final String FIELD_TARGET_SITE_NAME = "target_site";
        public static final String FIELD_RECORD_START_ID = "start_id";
        public static final String FIELD_RECORD_END_ID = "end_id";
    }

    public static class GlobalConf {
        public static final String FIELD_GLOBAL_CONF_KEY = "conf_key";
        public static final String FIELD_GLOBAL_CONF_VALUE = "conf_value";
        public static final String FIELD_GLOBAL_CONF_DESC = "conf_desc";
    }
}
