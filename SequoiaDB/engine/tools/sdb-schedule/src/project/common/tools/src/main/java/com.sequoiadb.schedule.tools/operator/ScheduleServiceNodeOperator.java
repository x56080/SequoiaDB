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

   Source File Name = ScheduleServiceNodeOperator.java

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
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;

public abstract class ScheduleServiceNodeOperator {
    public static String HEALTH_STATUS_UP = "UP";
    private static Logger logger = LoggerFactory.getLogger(ScheduleServiceNodeOperator.class);

    public abstract void init(String installPath) throws ScheduleToolsException;

    // 启动本机本服务的所有节点，非阻塞启动
    public void startAll() throws ScheduleToolsException {
        List<ScheduleNodeInfoDetail> allNode = getAllNodeInfoDetail();
        for (ScheduleNodeInfoDetail node : allNode) {
            if (node.getPid() != ScheduleNodeInfoDetail.NOT_RUNNING) {
                continue;
            }

            try {
                start(node.getNodeInfo().getPort());
            }
            catch (ScheduleToolsException e) {
                logger.error("failed to start node: type={}, port={}",
                        node.getNodeInfo().getNodeType(), node.getNodeInfo().getPort(), e);
            }
        }
    }

    // 启动本机的制定节点，非阻塞启动
    public abstract void start(int port) throws ScheduleToolsException;

    // 停止本机的指定节点，非阻塞停止
    public abstract void stop(int port, boolean force) throws ScheduleToolsException;

    // 停止本机本服务的所有节点，非阻塞停止
    public void stopAll(boolean force) throws ScheduleToolsException {
        List<ScheduleNodeInfoDetail> allNode = getAllNodeInfoDetail();
        for (ScheduleNodeInfoDetail node : allNode) {
            if (node.getPid() == ScheduleNodeInfoDetail.NOT_RUNNING) {
                continue;
            }

            try {
                stop(node.getNodeInfo().getPort(), force);
            }
            catch (ScheduleToolsException e) {
                logger.error("failed to start node: type={}, port={}",
                        node.getNodeInfo().getNodeType(), node.getNodeInfo().getPort(), e);
            }
        }
    }

    public abstract List<ScheduleNodeInfoDetail> getAllNodeInfoDetail() throws ScheduleToolsException;

    // 返回空如果节点没找到
    public abstract ScheduleNodeInfoDetail getNodeInfoDetail(int port) throws ScheduleToolsException;

    // 返回空如果节点没找到
    public abstract ScheduleNodeInfo getNodeInfo(int port) throws ScheduleToolsException;

    public abstract List<ScheduleNodeInfo> getAllNodeInfo() throws ScheduleToolsException;

    // 返回 HEALTH_STATUS_UP 表示节点处于健康状态，其它表示不健康
    public abstract String getHealthDesc(int port);

    public abstract ScheduleNodeType getNodeType();

}
