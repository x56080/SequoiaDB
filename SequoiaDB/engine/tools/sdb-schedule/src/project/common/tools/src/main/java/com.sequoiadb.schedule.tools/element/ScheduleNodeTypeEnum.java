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

   Source File Name = ScheduleNodeTypeEnum.java

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

public enum ScheduleNodeTypeEnum {
    SCHEDULESERVER("schedule-server", "1", "schedule-server-", 1);

    private final String name;
    private final String typeNum;
    private final String jarNamePrefix;
    private final int deployPriority;

    ScheduleNodeTypeEnum(String name, String typeNum, String jarNamePrefix, int deployPriority) {
        this.name = name;
        this.typeNum = typeNum;
        this.jarNamePrefix = jarNamePrefix;
        this.deployPriority = deployPriority;
    }

    public String getName() {
        return name;
    }

    public String getTypeNum() {
        return typeNum;
    }

    public String getJarNamePrefix() {
        return jarNamePrefix;
    }

    public int getDeployPriority() {
        return deployPriority;
    }

    public static ScheduleNodeTypeEnum getNodeByName(String name) {
        for (ScheduleNodeTypeEnum nodeType : ScheduleNodeTypeEnum.values()) {
            if (nodeType.getName().equals(name)) {
                return nodeType;
            }
        }
        throw new IllegalArgumentException(name + "not exit schedule service");
    }

}
