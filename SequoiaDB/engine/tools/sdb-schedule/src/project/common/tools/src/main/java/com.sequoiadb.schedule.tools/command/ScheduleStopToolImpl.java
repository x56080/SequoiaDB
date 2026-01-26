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

   Source File Name = ScheduleStopToolImpl.java

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
import com.sequoiadb.schedule.tools.common.NodeStopStatus;
import com.sequoiadb.schedule.tools.common.ScheduleCommandUtil;
import com.sequoiadb.schedule.tools.common.ScheduleCommon;
import com.sequoiadb.schedule.tools.common.ScheduleHelpGenerator;
import com.sequoiadb.schedule.tools.common.ScheduleHelper;
import com.sequoiadb.schedule.tools.common.ScheduleToolsDefine;
import com.sequoiadb.schedule.tools.element.ScheduleNodeInfo;
import com.sequoiadb.schedule.tools.element.ScheduleNodeInfoDetail;
import com.sequoiadb.schedule.tools.element.ScheduleNodeType;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import com.sequoiadb.schedule.tools.operator.ScheduleServiceNodeOperator;
import com.sequoiadb.schedule.tools.operator.ScheduleServiceNodeOperatorGroup;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ScheduleStopToolImpl extends ScheduleTool {
    protected final String OPT_LONG_FORCE = "force";
    protected final String OPT_SHORT_FORCE = "f";
    private final ScheduleServiceNodeOperatorGroup nodeOperators;
    private final String installPath;

    protected int SLEEP_TIME = 200;
    protected int STOP_TIMEOUT = 30 * 1000;
    protected int STOP_INTERVAL_TIME = 100;

    protected Options options;
    protected ScheduleHelpGenerator hp;
    protected int success = 0;
    protected static Logger logger = LoggerFactory.getLogger(ScheduleStopToolImpl.class);

    public ScheduleStopToolImpl(List<ScheduleServiceNodeOperator> nodeOperatorList) throws ScheduleToolsException {
        super("stop");
        this.nodeOperators = new ScheduleServiceNodeOperatorGroup(nodeOperatorList);
        this.installPath = ScheduleHelper
                .getAbsolutePathFromTool(ScheduleHelper.getPwd() + File.separator + "..");
        nodeOperators.init(installPath);

        options = new Options();
        hp = new ScheduleHelpGenerator();
        options.addOption(
                hp.createOpt(ScheduleCommandUtil.OPT_SHORT_PORT, ScheduleCommandUtil.OPT_LONG_PORT,
                        "node port.", false, true, false));

        ScheduleCommandUtil.addTypeOptionForStartOrStop(nodeOperators.getSupportTypes(), options, hp,
                false, true);

        options.addOption(hp.createOpt(OPT_SHORT_FORCE, OPT_LONG_FORCE, "force to stop node.",
                false, false, false));
    }

    @Override
    public void process(String[] args) throws ScheduleToolsException {
        // 日志
        ScheduleHelper.configToolsLog(ScheduleToolsDefine.FILE_NAME.STOP_LOG_CONF);
        CommandLine commandLine = ScheduleCommandUtil.parseArgs(args, options);
        if (commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_PORT)
                && commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_NODE_TYPE)
                || !commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_PORT)
                        && !commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_NODE_TYPE)) {
            logger.error("Invalid arg:please set -t or -p");
            throw new ScheduleToolsException("please set -t or -p", ScheduleBaseExitCode.INVALID_ARG);
        }

        Map<Integer, ScheduleNodeInfo> needStopMap = new HashMap<Integer, ScheduleNodeInfo>();
        // -p node
        if (commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_PORT)) {
            try {
                String portString = commandLine.getOptionValue(ScheduleCommandUtil.OPT_SHORT_PORT);
                int port = ScheduleCommon.convertStrToInt(portString);
                ScheduleNodeInfo nodeInfo = nodeOperators.getNodeInfo(port);
                needStopMap.put(port, nodeInfo);
            }
            catch (ScheduleToolsException e) {
                e.printErrorMsg();
                logger.error("failed to stop node="
                        + commandLine.getOptionValue(ScheduleCommandUtil.OPT_SHORT_PORT),
                        e);
                System.out.println("Total:1;Success:0;Failed:1");
                logger.info("Total:1;Success:0;Failed:1");
                throw new ScheduleToolsException(e.getExitCode());
            }
        }

        // --all
        else if (commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_NODE_TYPE)) {
            String type = commandLine.getOptionValue(ScheduleCommandUtil.OPT_SHORT_NODE_TYPE).trim();
            if (type.equals(ScheduleToolsDefine.NODE_TYPE.ALL_NUM)
                    || type.equals(ScheduleToolsDefine.NODE_TYPE.ALL_STR)) {
                needStopMap.putAll(nodeOperators.getAllNode());
                type = "all";
            }
            else {
                ScheduleNodeType typeEnum = nodeOperators.getSupportTypes().getNodeTypeByStr(type);
                Map<Integer, ScheduleNodeInfo> typeNodes = nodeOperators.getNodesByType(typeEnum);
                if (typeNodes != null) {
                    needStopMap.putAll(typeNodes);
                }
                type = typeEnum.toString();
            }
            if (needStopMap.size() <= 0) {
                System.out.println("Can't find any node in conf path,type:" + type);
                logger.info("Can't find any node in conf path,stop success,type:" + type);
                System.out.println("Total:" + needStopMap.size() + ";Success:0;Failed:0");
                return;
            }
        }

        stopNodes(needStopMap, commandLine.hasOption(OPT_SHORT_FORCE));
    }

    protected void stopNodes(Map<Integer, ScheduleNodeInfo> needStopMap, boolean isForce)
            throws ScheduleToolsException {
        List<ScheduleNodeInfo> nodeInfoList = getSortedStopList(needStopMap);
        List<ScheduleNodeInfo> timeOutList = new ArrayList<>();
        List<ScheduleNodeInfo> checkStatusList = new ArrayList<>();
        for (ScheduleNodeInfo node : nodeInfoList) {
            try {
                if (nodeOperators.getNodePid(node.getPort()) != ScheduleNodeInfoDetail.NOT_RUNNING) {
                    // not force: kill -15 pid
                    nodeOperators.stopNode(node.getPort(), false);
                    checkStatusList.add(node);
                    // 睡眠 100 ms，尽量避免后执行stop的节点比先执行stop的节点先停止，导致出错
                    ScheduleCommon.sleep(STOP_INTERVAL_TIME);
                }
                else {
                    logger.info("Success:" + node.getNodeType().getUpperName() + "("
                            + node.getPort() + ")" + " is already stopped");
                    System.out.println("Success:" + node.getNodeType().getUpperName() + "("
                            + node.getPort() + ")" + " is already stopped");
                    success++;
                }
            }
            catch (ScheduleToolsException e) {
                logger.error("Failed:" + node.getNodeType().getUpperName() + "(" + node.getPort()
                        + ")" + " failed to stop, stop occur error", e);
                System.out.println("Failed:" + node.getNodeType().getUpperName() + "("
                        + node.getPort() + ")" + " failed to stop");
            }
        }

        for (ScheduleNodeInfo node : checkStatusList) {
            // check node status
            NodeStopStatus checkResult = checkNodeStoppingStatus(node, STOP_TIMEOUT);
            if (isForce && checkResult == NodeStopStatus.TIME_OUT) {
                // force: kill -15 pid still running then kill -9 pid
                stopForce(node);
                ScheduleCommon.sleep(SLEEP_TIME);
                checkResult = checkNodeStoppingStatus(node, 0);
            }

            if (checkResult == NodeStopStatus.NOT_RUNNING) {
                logger.info("Success:" + node.getNodeType().getUpperName() + "(" + node.getPort()
                        + ")" + " is successfully stopped");
                System.out.println("Success:" + node.getNodeType().getUpperName() + "("
                        + node.getPort() + ")" + " is successfully stopped");
                success++;
            }
            else if (checkResult == NodeStopStatus.FAILED) {
                logger.error("Failed:" + node.getNodeType().getUpperName() + "(" + node.getPort()
                        + ")" + " failed to stop, check node status failed, unknown node status");
                System.out.println(
                        "Failed:" + node.getNodeType().getUpperName() + "(" + node.getPort() + ")"
                                + " failed to stop, check node status failed, unknown node status");
            }
            else {
                timeOutList.add(node);
            }
        }

        // time out some node still running
        for (ScheduleNodeInfo node : timeOutList) {
            String nodeLog = ScheduleCommon.getServiceInstallPath() + File.separator + "log"
                    + File.separator + node.getNodeType().getName() + File.separator
                    + node.getPort() + File.separator
                    + (node.getNodeType().getName().replace("-", "")) + ".log";
            logger.error("Failed:" + node.getNodeType().getUpperName() + "(" + node.getPort() + ")"
                    + " failed to stop, timeout, node still running, check log for detail: "
                    + nodeLog);
            System.out.println("Failed:" + node.getNodeType().getUpperName() + "(" + node.getPort()
                    + ")" + " failed to stop");
        }

        System.out.println("Total:" + nodeInfoList.size() + ";Success:" + success + ";Failed:"
                + (nodeInfoList.size() - success));
        logger.info("Total:" + nodeInfoList.size() + ";Success:" + success + ";Failed:"
                + (nodeInfoList.size() - success));

        if (nodeInfoList.size() != success) {
            throw new ScheduleToolsException("please check log: " + ScheduleCommon.getStopLogPath(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
    }

    private NodeStopStatus checkNodeStoppingStatus(ScheduleNodeInfo node, int stopTimeOut) {
        long timeStamp = System.currentTimeMillis();
        while (true) {
            NodeStopStatus status = getNodeStopStatus(node);
            if (status == NodeStopStatus.NOT_RUNNING || status == NodeStopStatus.FAILED) {
                return status;
            }
            if (System.currentTimeMillis() - timeStamp > stopTimeOut) {
                break;
            }
            ScheduleCommon.sleep(SLEEP_TIME);
        }
        return NodeStopStatus.TIME_OUT;
    }

    private void stopForce(ScheduleNodeInfo node) throws ScheduleToolsException {
        // force stop node
        try {
            logger.info("force stop node:" + node.getNodeType().getUpperName() + "("
                    + node.getPort() + ")");
            nodeOperators.stopNode(node.getPort(), true);
        }
        catch (ScheduleToolsException e) {
            logger.error("force stop node occur exception,node:" + node.getNodeType().getUpperName()
                    + "(" + node.getPort() + ")", e);
            System.out.println(
                    "force stop node occur exception,node:" + node.getNodeType().getUpperName()
                            + "(" + node.getPort() + ")" + ",error:" + e.getMessage());
            throw e;
        }
    }

    private NodeStopStatus getNodeStopStatus(ScheduleNodeInfo node) {
        try {
            int pid = nodeOperators.getNodePid(node.getPort());
            if (pid == ScheduleNodeInfoDetail.NOT_RUNNING) {
                return NodeStopStatus.NOT_RUNNING;
            }
            return NodeStopStatus.RUNNING;
        }
        catch (ScheduleToolsException e) {
            logger.error("check node status failed,node:" + node.getNodeType().getUpperName() + "("
                    + node.getPort() + ")", e);
            return NodeStopStatus.FAILED;
        }
    }

    protected List<ScheduleNodeInfo> getSortedStopList(Map<Integer, ScheduleNodeInfo> needStopMap) {
        Collection<ScheduleNodeInfo> values = needStopMap.values();
        List<ScheduleNodeInfo> nodeInfoList = new ArrayList<>(values);
        // 优先级越小，优先部署，逆序排序，先部署的后停止
        Collections.sort(nodeInfoList, new Comparator<ScheduleNodeInfo>() {
            @Override
            public int compare(ScheduleNodeInfo n1, ScheduleNodeInfo n2) {
                return n2.getNodeType().getTypeEnum().getDeployPriority()
                        - n1.getNodeType().getTypeEnum().getDeployPriority();
            }
        });
        return nodeInfoList;
    }

    @Override
    public void printHelp(boolean isFullHelp) {
        hp.printHelp(isFullHelp);
    }
}
