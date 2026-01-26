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

   Source File Name = ScheduleJobManagerRefresher.java

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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ScheduleJobManagerRefresher {

    private static final Logger logger = LoggerFactory.getLogger(ScheduleJobManagerRefresher.class);

    public static void refreshThreadPoolConfig(int coreSize, int maxSize) {
        ScheduleJobManager jobManager = ScheduleJobManager.getInstance();
        if (jobManager.getCoreThreadSize() != coreSize
                || jobManager.getMaxThreadSize() != maxSize) {
            jobManager.updateThreadPoolConfig(coreSize, maxSize);
            logger.info("update ScheduleJobManager config:coreSize={},maxSize={}", coreSize,
                    maxSize);
        }
    }
}
