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

   Source File Name = DefaultNodeOperator.java

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

import com.sequoiadb.schedule.tools.common.IOUtils;
import com.sequoiadb.schedule.tools.element.ScheduleNodeInfo;
import com.sequoiadb.schedule.tools.element.ScheduleNodeInfoDetail;
import com.sequoiadb.schedule.tools.element.ScheduleNodeProcessInfo;
import com.sequoiadb.schedule.tools.element.ScheduleNodeType;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import com.sequoiadb.schedule.tools.exec.ExecutorWrapper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

public class DefaultNodeOperator extends ScheduleServiceNodeOperator {
    private final ScheduleNodeType nodeType;
    private ExecutorWrapper executor;
    private final List<String> healthEndpoints;
    private String installPath;
    private static final Logger logger = LoggerFactory.getLogger(DefaultNodeOperator.class);

    public DefaultNodeOperator(ScheduleNodeType nodeType, String... healthEndpoints)
            throws ScheduleToolsException {
        this.nodeType = nodeType;
        this.healthEndpoints = Arrays.asList(healthEndpoints);
    }

    public DefaultNodeOperator(ScheduleNodeType nodeType) throws ScheduleToolsException {
        this(nodeType, "internal/v1/health", "/health");
    }

    @Override
    public void init(String installPath) throws ScheduleToolsException {
        this.installPath = installPath;
        this.executor = new ExecutorWrapper(nodeType, installPath);
    }

    // 启动本机的制定节点，非阻塞启动
    @Override
    public void start(int port) throws ScheduleToolsException {
        ScheduleNodeInfo node = executor.getNodeCheck(port);
        executor.startNode(node);
    }

    // 停止本机的指定节点，非阻塞停止
    @Override
    public void stop(int port, boolean force) throws ScheduleToolsException {
        executor.stopNode(port, force);
    }

    @Override
    public List<ScheduleNodeInfoDetail> getAllNodeInfoDetail() throws ScheduleToolsException {
        ArrayList<ScheduleNodeInfoDetail> ret = new ArrayList<>();
        Map<Integer, ScheduleNodeInfo> allNode = executor.getAllNode();
        Map<String, ScheduleNodeProcessInfo> runningNode = executor.getNodeStatus();
        for (ScheduleNodeInfo nodeInfo : allNode.values()) {
            ScheduleNodeProcessInfo processInfo = runningNode.get(nodeInfo.getConfPath());
            if (processInfo == null) {
                ret.add(new ScheduleNodeInfoDetail(nodeInfo, ScheduleNodeInfoDetail.NOT_RUNNING));
                continue;
            }
            ret.add(new ScheduleNodeInfoDetail(nodeInfo, processInfo.getPid()));
        }

        return ret;
    }

    @Override
    public ScheduleNodeInfoDetail getNodeInfoDetail(int port) throws ScheduleToolsException {
        ScheduleNodeInfo nodeInfo = executor.getNode(port);
        if (nodeInfo == null) {
            return null;
        }
        Map<String, ScheduleNodeProcessInfo> runningNode = executor.getNodeStatus();
        ScheduleNodeProcessInfo processInfo = runningNode.get(nodeInfo.getConfPath());
        if (processInfo == null) {
            return new ScheduleNodeInfoDetail(nodeInfo, ScheduleNodeInfoDetail.NOT_RUNNING);

        }
        return new ScheduleNodeInfoDetail(nodeInfo, processInfo.getPid());
    }

    @Override
    public String getHealthDesc(int port) {
        Socket s = new Socket();
        try {
            s.connect(new InetSocketAddress("localhost", port), 5000);
            return "UP";
        } catch (Exception e) {
            return e.getMessage() + ", stacktrace: " + Arrays.toString(e.getStackTrace());
        } finally {
            IOUtils.close(s);
        }
    }

    @Override
    public ScheduleNodeType getNodeType() {
        return nodeType;
    }

    @Override
    public List<ScheduleNodeInfo> getAllNodeInfo() throws ScheduleToolsException {
        Map<Integer, ScheduleNodeInfo> allNode = executor.getAllNode();
        ArrayList<ScheduleNodeInfo> ret = new ArrayList<>();
        ret.addAll(allNode.values());
        return ret;
    }

    @Override
    public ScheduleNodeInfo getNodeInfo(int port) throws ScheduleToolsException {
        return executor.getNode(port);
    }
}
