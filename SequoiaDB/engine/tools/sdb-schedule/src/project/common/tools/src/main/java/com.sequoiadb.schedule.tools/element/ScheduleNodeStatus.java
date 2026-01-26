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

   Source File Name = ScheduleNodeStatus.java

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

import java.util.HashMap;
import java.util.Map;

public class ScheduleNodeStatus {
    private Map<String, ScheduleNodeProcessInfo> conf2NodeProcessInfo = new HashMap<>();
    private Map<ScheduleNodeType, Map<String, ScheduleNodeProcessInfo>> type2NodeProcessInfo = new HashMap<>();

    public void addNode(ScheduleNodeProcessInfo node) {
        conf2NodeProcessInfo.put(node.getConf(), node);
        Map<String, ScheduleNodeProcessInfo> nodeInfos = type2NodeProcessInfo.get(node.getType());
        if (nodeInfos == null) {
            nodeInfos = new HashMap<String, ScheduleNodeProcessInfo>();
            type2NodeProcessInfo.put(node.getType(), nodeInfos);
        }
        nodeInfos.put(node.getConf(), node);
    }

    public Map<String, ScheduleNodeProcessInfo> getStatusMap() {
        return conf2NodeProcessInfo;
    }

}
