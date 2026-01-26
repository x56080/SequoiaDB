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

   Source File Name = ScheduleLeaderAction.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core.elect;

import com.sequoiadb.schedule.common.ScheduleCommonTools;
import com.sequoiadb.schedule.core.ScheduleMgrWrapper;
import com.sequoiadb.schedule.core.TaskStatusMgr;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.util.backoff.BackOff;
import org.springframework.util.backoff.BackOffExecution;

public class ScheduleLeaderAction implements LeaderAction {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleLeaderAction.class);

    public ScheduleLeaderAction() {
    }

    @Override
    public void run() {
        try {
            logger.info("################leader init#######################");
            ScheduleMgrWrapper.getInstance().start();
            TaskStatusMgr.getInstance().start();
            logger.info("################leader init done#######################");

        }
        catch (Exception e) {
            logger.error("failed to init schedule mgr, exit", e);
            ScheduleCommonTools.exitProcess();
        }
    }
}
