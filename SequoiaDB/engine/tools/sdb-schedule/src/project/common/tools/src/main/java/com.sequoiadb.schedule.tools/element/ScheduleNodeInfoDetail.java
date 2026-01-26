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

   Source File Name = ScheduleNodeInfoDetail.java

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

public class ScheduleNodeInfoDetail {
    public static final int NOT_RUNNING = -1;
    private ScheduleNodeInfo nodeInfo;
    private int pid = NOT_RUNNING;

    public ScheduleNodeInfoDetail(ScheduleNodeInfo nodeInfo, int pid) {
        this.nodeInfo = nodeInfo;
        this.pid = pid;
    }

    public ScheduleNodeInfo getNodeInfo() {
        return nodeInfo;
    }

    public int getPid() {
        return pid;
    }

    @Override
    public String toString() {
        return "ScheduleNodeInfoDetail{" + "nodeInfo=" + nodeInfo + ", pid=" + pid + '}';
    }
}
