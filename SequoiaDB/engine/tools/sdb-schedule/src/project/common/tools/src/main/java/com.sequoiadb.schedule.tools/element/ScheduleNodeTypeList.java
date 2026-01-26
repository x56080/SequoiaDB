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

   Source File Name = ScheduleNodeTypeList.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.tools.element;

import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;

import java.util.ArrayList;

public class ScheduleNodeTypeList extends ArrayList<ScheduleNodeType> {

    public ScheduleNodeType getNodeTypeByStr(String str) throws ScheduleToolsException {
        for (ScheduleNodeType nodeType : this) {
            if (nodeType.getName().equals(str) || nodeType.getType().equals(str)) {
                return nodeType;
            }
        }

        throw new ScheduleToolsException("unknown type:" + str, ScheduleBaseExitCode.INVALID_ARG);
    }
}
