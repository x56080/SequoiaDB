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

   Source File Name = ScheduleServerScriptEnum.java

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

import java.util.ArrayList;
import java.util.List;

public enum ScheduleServerScriptEnum {
    // 枚举类型根据每个节点工具的 list 命令输出定
    SCHEDULESERVER("sdb-schedule", "SCHEDULE-SERVER", "schctl.sh");

    private final String dirName;
    private final String type;
    private final String shellName;

    ScheduleServerScriptEnum(String dirName, String type, String shellName) {
        this.dirName = dirName;
        this.type = type;
        this.shellName = shellName;
    }

    public String getDirName() {
        return dirName;
    }

    public String getType() {
        return type;
    }

    public String getShellName() {
        return shellName;
    }

    public static String getShellNameByDirName(String dirName) {
        ScheduleServerScriptEnum[] enums = ScheduleServerScriptEnum.values();
        for (ScheduleServerScriptEnum e : enums) {
            if (e.getDirName().equals(dirName)) {
                return e.getShellName();
            }
        }
        return null;
    }

    public static List<String> getAllType() {
        List<String> typeList = new ArrayList<>();
        ScheduleServerScriptEnum[] enums = ScheduleServerScriptEnum.values();
        for (ScheduleServerScriptEnum e : enums) {
            typeList.add(e.getType());
        }
        return typeList;
    }

    public static ScheduleServerScriptEnum getEnumByType(String type) {
        ScheduleServerScriptEnum[] enums = ScheduleServerScriptEnum.values();
        for (ScheduleServerScriptEnum e : enums) {
            if (type.toUpperCase().equals(e.getType())) {
                return e;
            }
        }
        return null;
    }

    public static ScheduleServerScriptEnum getEnumByDirName(String dirName) {
        ScheduleServerScriptEnum[] enums = ScheduleServerScriptEnum.values();
        for (ScheduleServerScriptEnum e : enums) {
            if (dirName.equals(e.getDirName())) {
                return e;
            }
        }
        return null;
    }
}
