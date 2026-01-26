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

   Source File Name = SchCtl.java

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
import com.sequoiadb.schedule.tools.command.ScheduleListToolImpl;
import com.sequoiadb.schedule.tools.command.ScheduleStartToolImpl;
import com.sequoiadb.schedule.tools.command.ScheduleStopToolImpl;
import com.sequoiadb.schedule.tools.element.ScheduleNodeType;
import com.sequoiadb.schedule.tools.element.ScheduleNodeTypeEnum;
import com.sequoiadb.schedule.tools.element.ScheduleServerScriptEnum;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import com.sequoiadb.schedule.tools.operator.DefaultNodeOperator;
import com.sequoiadb.schedule.tools.operator.ScheduleServiceNodeOperator;

import java.util.Collections;
import java.util.List;

public class SchCtl {
    public static void main(String[] args) {
        CommandManager cmd = new CommandManager("schctl");
        try {
            List<ScheduleServiceNodeOperator> opList = Collections
                    .<ScheduleServiceNodeOperator> singletonList(new DefaultNodeOperator(new ScheduleNodeType(
                            ScheduleNodeTypeEnum.SCHEDULESERVER, ScheduleServerScriptEnum.SCHEDULESERVER)));
            cmd.addTool(new ScheduleStartToolImpl(opList));
            cmd.addTool(new ScheduleStopToolImpl(opList));
            cmd.addTool(new ScheduleListToolImpl(opList));
        }
        catch (ScheduleToolsException e) {
            e.printStackTrace();
            System.exit(e.getExitCode());
        }
        cmd.execute(args);
    }
}
