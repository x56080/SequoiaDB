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

   Source File Name = ScheduleStartToolImpl.java

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
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

public class ScheduleStartToolImpl extends ScheduleTool {
    private final int TIME_WAIT_PROCESS_RUNNING = 10000; // 10s
    private final ScheduleServiceNodeOperatorGroup nodeOperators;
    private final String installPath;
    protected int waitProcessTimeout = 50000; // 50s
    protected int SLEEP_TIME = 200;

    private final String OPT_SHORT_I = "I";
    private final String OPT_LONG_TIMEOUT = "timeout";
    // private final String OPT_LONG_OPTION = "option";
    private List<ScheduleNodeInfo> startSuccessList = new ArrayList<>();
    private static Logger logger = LoggerFactory.getLogger(ScheduleStartToolImpl.class);
    private ScheduleHelpGenerator hp;
    private Options options;

    public ScheduleStartToolImpl(List<ScheduleServiceNodeOperator> nodeOperatorList)
            throws ScheduleToolsException {
        super("start");
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

        options.addOption(hp.createOpt(OPT_SHORT_I, null, "use current user.", false, false, true));
        options.addOption(hp.createOpt(null, OPT_LONG_TIMEOUT,
                "sets the starting timeout in seconds, default:50", false, true, false));
    }

    @Override
    public void process(String[] args) throws ScheduleToolsException {
        // 日志
        ScheduleHelper.configToolsLog(ScheduleToolsDefine.FILE_NAME.START_LOG_CONF);

        CommandLine commandLine = ScheduleCommandUtil.parseArgs(args, options);
        if (commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_PORT)
                && commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_NODE_TYPE)
                || !commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_PORT)
                        && !commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_NODE_TYPE)) {
            logger.error("Invalid arg:please set -" + ScheduleCommandUtil.OPT_SHORT_NODE_TYPE + " or -"
                    + ScheduleCommandUtil.OPT_SHORT_PORT);
            throw new ScheduleToolsException(
                    "please set -" + ScheduleCommandUtil.OPT_SHORT_NODE_TYPE + " or -"
                            + ScheduleCommandUtil.OPT_SHORT_PORT,
                    ScheduleBaseExitCode.INVALID_ARG);
        }

        if (commandLine.hasOption(OPT_LONG_TIMEOUT)) {
            waitProcessTimeout = ScheduleCommandUtil.getTimeout(commandLine, OPT_LONG_TIMEOUT);
        }


        Map<Integer, ScheduleNodeInfo> needStartMap = new HashMap<Integer, ScheduleNodeInfo>();
        // -p
        if (commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_PORT)) {
            String portString = commandLine.getOptionValue(ScheduleCommandUtil.OPT_SHORT_PORT);
            try {
                int port = ScheduleCommon.convertStrToInt(portString);
                ScheduleNodeInfo node = nodeOperators.getNodeInfo(port);
                needStartMap.put(port, node);
            }
            catch (ScheduleToolsException e) {
                e.printErrorMsg();
                logger.error("Failed to start " + portString, e);
                System.out.println("Total:1;Success:0;Failed:1");
                throw new ScheduleToolsException(e.getExitCode());
            }
        }

        // -t
        else if (commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_NODE_TYPE)) {
            String type = commandLine.getOptionValue(ScheduleCommandUtil.OPT_SHORT_NODE_TYPE).trim();
            if (type.equals(ScheduleToolsDefine.NODE_TYPE.ALL_NUM)
                    || type.equals(ScheduleToolsDefine.NODE_TYPE.ALL_STR)) {
                needStartMap.putAll(nodeOperators.getAllNode());
                type = ScheduleToolsDefine.NODE_TYPE.ALL_STR;
            }
            else {
                ScheduleNodeType typeEnum = this.nodeOperators.getSupportTypes().getNodeTypeByStr(type);
                Map<Integer, ScheduleNodeInfo> typeNodes = nodeOperators.getNodesByType(typeEnum);
                if (typeNodes != null) {
                    needStartMap.putAll(typeNodes);
                }
                type = typeEnum.toString();
            }

            if (needStartMap.size() <= 0) {
                System.out.println("Can't find any node in conf path,type:" + type);
                System.out.println("Total:0;Success:0;Failed:0");
                logger.info("Can't find any node in conf path,start all node success,type=" + type);
                return;
            }
        }

        List<ScheduleNodeInfo> checkList = new ArrayList<>();
        startNodes(needStartMap, checkList);

        // check start res
        boolean startRes = isStartSuccess(checkList);

        logger.info("Total:" + needStartMap.size() + ";Success:" + startSuccessList.size()
                + ";Failed:" + (needStartMap.size() - startSuccessList.size()));
        System.out.println("Total:" + needStartMap.size() + ";Success:" + startSuccessList.size()
                + ";Failed:" + (needStartMap.size() - startSuccessList.size()));
        if (!startRes || needStartMap.size() - startSuccessList.size() > 0) {
            throw new ScheduleToolsException("please check log: " + ScheduleCommon.getStartLogPath(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
    }

    protected void startNodes(Map<Integer, ScheduleNodeInfo> needStartMap, List<ScheduleNodeInfo> checkList) {
        for (Integer key : needStartMap.keySet()) {
            try {
                int pid = nodeOperators.getNodePid(key);
                if (pid == ScheduleNodeInfoDetail.NOT_RUNNING) {
                    nodeOperators.startNode(needStartMap.get(key).getPort());
                    checkList.add(needStartMap.get(key));
                }
                else {
                    String status = nodeOperators.getHealthDesc(key);
                    if (status.equals(ScheduleServiceNodeOperator.HEALTH_STATUS_UP)) {
                        System.out.println(
                                "Success:" + needStartMap.get(key).getNodeType().getUpperName()
                                        + "(" + needStartMap.get(key).getPort() + ")"
                                        + " is already started (" + pid + ")");
                        logger.info("Success:" + needStartMap.get(key).getNodeType().getUpperName()
                                + "(" + needStartMap.get(key).getPort() + ")"
                                + " is already started (" + pid + ")");
                        startSuccessList.add(needStartMap.get(key));
                    }
                    else {
                        System.out.println("Failed:"
                                + needStartMap.get(key).getNodeType().getUpperName() + "("
                                + needStartMap.get(key).getPort() + ")" + " is already started ("
                                + pid + "),but node status is not normal");
                        logger.info("Failed:" + needStartMap.get(key).getNodeType().getUpperName()
                                + "(" + needStartMap.get(key).getPort() + ")"
                                + " is already started (" + pid + "),but node status is not normal:"
                                + status);
                    }
                }
            }
            catch (ScheduleToolsException e) {
                logger.error(e.getMessage() + ",errorCode:" + e.getExitCode(), e);
                e.printErrorMsg();
            }
        }
    }

    protected boolean isStartSuccess(List<ScheduleNodeInfo> checkList) throws ScheduleToolsException {
        long timeStamp = System.currentTimeMillis();
        boolean rc = true;
        Map<ScheduleNodeInfo, String> port2Status = new HashMap<>();

        while (true) {
            boolean isPidExist = false;
            Iterator<ScheduleNodeInfo> it = checkList.iterator();
            while (it.hasNext()) {
                ScheduleNodeInfo node = it.next();
                try {
                    int pid = nodeOperators.getNodePid(node.getPort());
                    if (pid != ScheduleNodeInfoDetail.NOT_RUNNING) {
                        isPidExist = true;
                        String runningStatus = nodeOperators.getHealthDesc(node.getPort());
                        if (runningStatus.equals(ScheduleServiceNodeOperator.HEALTH_STATUS_UP)) {
                            System.out.println("Success:" + node.getNodeType().getUpperName() + "("
                                    + node.getPort() + ")" + " is successfully started (" + pid
                                    + ")");
                            logger.info("Success:" + node.getNodeType().getUpperName() + "("
                                    + node.getPort() + ")" + " is successfully started (" + pid
                                    + ")");
                            startSuccessList.add(node);
                            it.remove();
                            port2Status.remove(node);
                        }
                        else {
                            port2Status.put(node, runningStatus);
                        }
                    }
                    else {
                        port2Status.put(node, "can not find pid");
                    }
                }
                catch (ScheduleToolsException e) {
                    e.printErrorMsg();
                    logger.error("check node status failed,port:" + node.getPort(), e);
                    it.remove();
                    rc = false;
                }
            }
            if (checkList.size() <= 0) {
                return rc;
            }

            if (!isPidExist && System.currentTimeMillis() - timeStamp > TIME_WAIT_PROCESS_RUNNING) {
                break;
            }

            if (isPidExist && System.currentTimeMillis() - timeStamp > waitProcessTimeout) {
                break;
            }

            ScheduleCommon.sleep(SLEEP_TIME);
        }

        for (Entry<ScheduleNodeInfo, String> entry : port2Status.entrySet()) {
            logger.error("failed to start node {}({}) because of timeout",
                    entry.getKey().getNodeType().getUpperName(), entry.getKey().getPort());
            logger.error("node status: {}",
                    entry.getValue());
            String nodeLog = ScheduleCommon.getServiceInstallPath() + File.separator + "log" + File.separator
                    + entry.getKey().getNodeType().getName() + File.separator
                    + entry.getKey().getPort() + File.separator
                    + (entry.getKey().getNodeType().getName().replace("-", "")) + ".log";
            logger.error("check log for detail: {}", new File(nodeLog).getPath());
            System.out.println("Failed:" + entry.getKey().getNodeType().getUpperName() + "("
                    + entry.getKey().getPort() + ")" + " failed to start");
        }

        return false;
    }

    @Override
    public void printHelp(boolean isFullHelp) {
        hp.printHelp(isFullHelp);
    }

}
