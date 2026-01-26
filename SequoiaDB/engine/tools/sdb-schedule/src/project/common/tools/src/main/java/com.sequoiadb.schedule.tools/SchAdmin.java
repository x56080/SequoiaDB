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

   Source File Name = SchAdmin.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

package com.sequoiadb.schedule.tools;

import com.sequoiadb.schedule.tools.command.ScheduleCreateNodeToolImpl;
import com.sequoiadb.schedule.tools.command.ScheduleCreateSiteToolImpl;
import com.sequoiadb.schedule.tools.command.ScheduleDeleteNodeToolImpl;
import com.sequoiadb.schedule.tools.common.ScheduleHelper;
import com.sequoiadb.schedule.tools.common.ScheduleToolsDefine;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
public class SchAdmin {
    public static void main(String[] args) throws ScheduleToolsException {
        CommandManager cmd = new CommandManager("schadmin");
        try {
            cmd.addTool(new ScheduleCreateNodeToolImpl());
            cmd.addTool(new ScheduleDeleteNodeToolImpl());
            cmd.addTool(new ScheduleCreateSiteToolImpl());
        }
        catch (ScheduleToolsException e) {
            e.printStackTrace();
            System.exit(e.getExitCode());
        }
        ScheduleHelper.configToolsLog(ScheduleToolsDefine.FILE_NAME.ADMIN_LOG_CONF);

        cmd.execute(args);
    }
}