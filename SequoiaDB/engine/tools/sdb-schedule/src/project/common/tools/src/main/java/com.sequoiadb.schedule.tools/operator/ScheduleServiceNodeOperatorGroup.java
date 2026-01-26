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

   Source File Name = ScheduleServiceNodeOperatorGroup.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.tools.operator;

import com.sequoiadb.schedule.tools.element.ScheduleNodeInfo;
import com.sequoiadb.schedule.tools.element.ScheduleNodeInfoDetail;
import com.sequoiadb.schedule.tools.element.ScheduleNodeType;
import com.sequoiadb.schedule.tools.element.ScheduleNodeTypeList;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ScheduleServiceNodeOperatorGroup {
    private Map<ScheduleNodeType, ScheduleServiceNodeOperator> operatorMap = new HashMap<>();
    private ScheduleNodeTypeList typeList = new ScheduleNodeTypeList();

    public ScheduleServiceNodeOperatorGroup(List<ScheduleServiceNodeOperator> operators) {
        for (ScheduleServiceNodeOperator op : operators) {
            operatorMap.put(op.getNodeType(), op);
            typeList.add(op.getNodeType());
        }
    }

    public ScheduleNodeTypeList getSupportTypes() {
        return typeList;
    }

    public ScheduleNodeInfo getNodeInfo(int port) throws ScheduleToolsException {
        for (ScheduleServiceNodeOperator op : operatorMap.values()) {
            ScheduleNodeInfo node = op.getNodeInfo(port);
            if (node != null) {
                return node;
            }
        }
        throw new ScheduleToolsException("no such node: port=" + port, ScheduleBaseExitCode.INVALID_ARG);
    }

    public ScheduleNodeInfoDetail getNodeDetail(int port) throws ScheduleToolsException {
        for (ScheduleServiceNodeOperator op : operatorMap.values()) {
            ScheduleNodeInfoDetail node = op.getNodeInfoDetail(port);
            if (node != null) {
                return node;
            }
        }
        throw new ScheduleToolsException("no such node: port=" + port, ScheduleBaseExitCode.INVALID_ARG);
    }

    public Map<Integer, ScheduleNodeInfo> getAllNode() throws ScheduleToolsException {
        Map<Integer, ScheduleNodeInfo> ret = new HashMap<>();
        for (ScheduleServiceNodeOperator op : operatorMap.values()) {
            List<ScheduleNodeInfo> nodeInfoList = op.getAllNodeInfo();
            for (ScheduleNodeInfo node : nodeInfoList) {
                ret.put(node.getPort(), node);
            }
        }
        return ret;
    }

    public List<ScheduleNodeInfoDetail> getAllNodeDetail() throws ScheduleToolsException {
        List<ScheduleNodeInfoDetail> ret = new ArrayList<>();
        for (ScheduleServiceNodeOperator op : operatorMap.values()) {
            List<ScheduleNodeInfoDetail> nodes = op.getAllNodeInfoDetail();
            ret.addAll(nodes);
        }
        return ret;
    }

    public Map<Integer, ScheduleNodeInfo> getNodesByType(ScheduleNodeType type) throws ScheduleToolsException {
        ScheduleServiceNodeOperator op = getOperator(type);

        Map<Integer, ScheduleNodeInfo> ret = new HashMap<>();
        List<ScheduleNodeInfo> nodeInfoList = op.getAllNodeInfo();
        for (ScheduleNodeInfo node : nodeInfoList) {
            ret.put(node.getPort(), node);
        }
        return ret;

    }

    public void init(String installPath) throws ScheduleToolsException {
        for (ScheduleServiceNodeOperator op : operatorMap.values()) {
            op.init(installPath);
        }
    }

    public int getNodePid(int port) throws ScheduleToolsException {
        for (ScheduleServiceNodeOperator op : operatorMap.values()) {
            ScheduleNodeInfoDetail detail = op.getNodeInfoDetail(port);
            if (detail != null) {
                return detail.getPid();
            }
        }
        throw new ScheduleToolsException("no such node: port=" + port, ScheduleBaseExitCode.INVALID_ARG);
    }

    public void startNode(int port) throws ScheduleToolsException {
        ScheduleNodeInfo node = getNodeInfo(port);
        ScheduleServiceNodeOperator op = getOperator(node.getNodeType());
        op.start(port);
    }

    public void stopNode(int port, boolean force) throws ScheduleToolsException {
        ScheduleNodeInfo node = getNodeInfo(port);
        ScheduleServiceNodeOperator op = getOperator(node.getNodeType());
        op.stop(port, force);
    }

    private ScheduleServiceNodeOperator getOperator(ScheduleNodeType type) throws ScheduleToolsException {
        ScheduleServiceNodeOperator op = operatorMap.get(type);
        if (op == null) {
            throw new ScheduleToolsException("no such type: type=" + type.getName(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        return op;
    }

    public String getHealthDesc(int port) throws ScheduleToolsException {
        ScheduleNodeInfo node = getNodeInfo(port);
        ScheduleServiceNodeOperator op = getOperator(node.getNodeType());
        return op.getHealthDesc(port);
    }
}
