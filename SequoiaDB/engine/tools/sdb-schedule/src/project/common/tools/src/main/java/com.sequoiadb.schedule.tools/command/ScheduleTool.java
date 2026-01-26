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

   Source File Name = ScheduleTool.java

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

import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;

public abstract class ScheduleTool {
    private String tooName = null;
    private boolean isHidden = false;

    public ScheduleTool(String tooName) {
        this.tooName = tooName;
    }

    public ScheduleTool(String tooName, boolean isHidden) {
        this.tooName = tooName;
        this.isHidden = isHidden;
    }

    public abstract void process(String[] args) throws ScheduleToolsException;

    public abstract void printHelp(boolean isFullHelp) throws ScheduleToolsException;

    public String getToolName() {
        return this.tooName;
    }

    public boolean isHidden() {
        return isHidden;
    }
}
