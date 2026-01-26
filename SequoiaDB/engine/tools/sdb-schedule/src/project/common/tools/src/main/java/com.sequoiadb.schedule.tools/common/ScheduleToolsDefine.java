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

   Source File Name = ScheduleToolsDefine.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

package com.sequoiadb.schedule.tools.common;

public class ScheduleToolsDefine {
    public static final String SYSTEM_JVM_OPTIONS = "system.jvm.options";

    public static class FILE_NAME {
        // ************dir*****************
        public static final String CONF = "conf";
        public static final String LOG = "log";
        public static final String JARS = "jars";

        // *************file******************
        public static final String APP_PROPS = "application.properties";
        public static final String LOGBACK = "logback.xml";
        public static final String ERROR_OUT = "error.out";


        // ***********tools log conf*************
        public static final String START_LOG_CONF = "logback_start.xml";
        public static final String STOP_LOG_CONF = "logback_stop.xml";
        public static final String ADMIN_LOG_CONF = "logback_admin.xml";

    }

    public static class PROPERTIES {
        public static final String SERVER_PORT = "server.port";
        public static final String LOG_PATH_VALUE = "LOG_PATH_VALUE";
        public static final String LOG_NAME_VALUE = "LOG_NAME_VALUE";
        public static final String APPLICATION_PROPERTIES_LOCATION = "spring.config.location";
    }

    public static class NODE_TYPE {
        // no enum
        public static final String ALL_NUM = "0";
        public static final String ALL_STR = "all";
    }
}
