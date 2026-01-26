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

   Source File Name = ScheduleListToolImpl.java

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
import com.sequoiadb.schedule.tools.element.ScheduleNodeInfo;
import com.sequoiadb.schedule.tools.element.ScheduleNodeInfoDetail;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import com.sequoiadb.schedule.tools.operator.ScheduleServiceNodeOperator;
import com.sequoiadb.schedule.tools.operator.ScheduleServiceNodeOperatorGroup;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class ScheduleListToolImpl extends ScheduleTool {
    private final ScheduleServiceNodeOperatorGroup nodeOperators;
    private final String installPath;
    private Options options;
    private ScheduleHelpGenerator hp;

    private final String OPT_VALUE_LOCAL = "local";
    private final String OPT_VALUE_RUN = "run";

    public ScheduleListToolImpl(List<ScheduleServiceNodeOperator> nodeOperatorList) throws ScheduleToolsException {
        super("list");
        this.nodeOperators = new ScheduleServiceNodeOperatorGroup(nodeOperatorList);
        this.installPath = ScheduleHelper
                .getAbsolutePathFromTool(ScheduleHelper.getPwd() + File.separator + "..");
        nodeOperators.init(installPath);

        options = new Options();
        hp = new ScheduleHelpGenerator();
        options.addOption(hp.createOpt(ScheduleCommandUtil.OPT_SHORT_PORT, ScheduleCommandUtil.OPT_LONG_PORT,
                "node port", false, true, false));
        options.addOption(hp.createOpt(ScheduleCommandUtil.OPT_SHORT_MODE, ScheduleCommandUtil.OPT_LONG_MODE,
                "list mode, 'run' or 'local', default:run.",
                false, true, false));
        options.addOption(hp.createOpt(ScheduleCommandUtil.OPT_SHORT_LONG, ScheduleCommandUtil.OPT_LONG_LONG,
                "show long style", false, false, true));
    }

    @Override
    public void process(String[] args) throws ScheduleToolsException {
        // TODO 日志
        CommandLine commandLine = ScheduleCommandUtil.parseArgs(args, options);
        boolean printRunningOnly = true;
        if (commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_MODE)) {
            String modeValue = commandLine.getOptionValue(ScheduleCommandUtil.OPT_SHORT_MODE);
            if (modeValue.equals(OPT_VALUE_LOCAL)) {
                printRunningOnly = false;
            }
            else if (!modeValue.equals(OPT_VALUE_RUN)) {
                throw new ScheduleToolsException("Unknown mode:" + modeValue,
                        ScheduleBaseExitCode.INVALID_ARG);
            }
        }

        List<ScheduleNodeInfo> nodeList = new ArrayList<>();
        List<String> pidList = new ArrayList<>();

        if (commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_PORT)) {
            String portStr = commandLine.getOptionValue(ScheduleCommandUtil.OPT_SHORT_PORT);
            int port = ScheduleCommon.convertStrToInt(portStr);
            ScheduleNodeInfoDetail node = nodeOperators.getNodeDetail(port);
            if (node != null) {
                if (node.getPid() != ScheduleNodeInfoDetail.NOT_RUNNING) {
                    nodeList.add(node.getNodeInfo());
                    pidList.add(node.getPid() + "");
                }
                else if (!printRunningOnly) {
                    nodeList.add(node.getNodeInfo());
                    pidList.add("-");
                }
            }
        }
        else {
            List<ScheduleNodeInfoDetail> allNodeDetail = nodeOperators.getAllNodeDetail();
            for (ScheduleNodeInfoDetail nodeInfoDetail : allNodeDetail) {
                if (nodeInfoDetail.getPid() != ScheduleNodeInfoDetail.NOT_RUNNING) {
                    nodeList.add(nodeInfoDetail.getNodeInfo());
                    pidList.add(nodeInfoDetail.getPid() + "");
                }
                else if (!printRunningOnly) {
                    nodeList.add(nodeInfoDetail.getNodeInfo());
                    pidList.add("-");
                }
            }
        }
        if (commandLine.hasOption(ScheduleCommandUtil.OPT_SHORT_LONG)) {
            for (int i = 0; i < pidList.size(); i++) {
                String propPath = nodeList.get(i).getConfPath() + File.separator + "application.properties";
                System.out.println(nodeList.get(i).getNodeType().getUpperName() + "("
                        + nodeList.get(i).getPort() + ")" + " (" + pidList.get(i) + ") "
                        + propPath);
            }
        }
        else {
            for (int i = 0; i < pidList.size(); i++) {
                System.out.println(nodeList.get(i).getNodeType().getUpperName() + "("
                        + nodeList.get(i).getPort() + ")" + " (" + pidList.get(i) + ")");
            }
        }
        System.out.println("Total:" + pidList.size());
        if (pidList.size() == 0) {
            throw new ScheduleToolsException(ScheduleBaseExitCode.EMPTY_OUT);
        }
    }

    @Override
    public void printHelp(boolean isFullHelp) {
        hp.printHelp(isFullHelp);
    }
}
