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

   Source File Name = ScheduleHelpFullToolImpl.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.tools.command;

import com.sequoiadb.schedule.tools.CommandManager;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;

public class ScheduleHelpFullToolImpl extends ScheduleTool {
    private CommandManager cmd;
    public ScheduleHelpFullToolImpl(CommandManager cmd) {
        super("helpfull");
        this.cmd = cmd;
    }

    @Override
    public void process(String[] args) throws ScheduleToolsException {
        this.cmd.checkHelpArgs(args);
        if (args.length >= 1) {
            this.cmd.printHelp(args[0], true);
        }
        else {
            System.out.println(this.cmd.getHelpMsg(true));
            System.exit(ScheduleBaseExitCode.SUCCESS);
        }
    }

    @Override
    public void printHelp(boolean isFullHelp) throws ScheduleToolsException {
        System.out.println(this.cmd.getHelpMsg(isFullHelp));
    }
}
