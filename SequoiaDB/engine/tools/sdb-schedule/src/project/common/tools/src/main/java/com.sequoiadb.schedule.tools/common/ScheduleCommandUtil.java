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

   Source File Name = ScheduleCommandUtil.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.tools.common;

import com.sequoiadb.schedule.tools.element.ScheduleNodeRequiredParamGroup;
import com.sequoiadb.schedule.tools.element.ScheduleNodeType;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.util.List;
import java.util.Map;

public class ScheduleCommandUtil {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleCommandUtil.class);

    public final static String OPT_SHORT_NODE_TYPE = "t";
    public final static String OPT_LONG_NODE_TYPE = "type";
    public final static String OPT_SHORT_PORT = "p";
    public final static String OPT_LONG_PORT = "port";
    public final static String OPT_SHORT_MODE = "m";
    public final static String OPT_LONG_MODE = "mode";
    public final static String OPT_SHORT_LONG = "l";
    public final static String OPT_LONG_LONG = "long";

    public static final String OPT_SHORT_HELP = "h";
    public static final String OPT_LONG_HELP = "help";
    public static final String OPT_LONG_VER = "version";
    public static final String OPT_SHORT_VER = "v";

    public static void addTypeOptionForStartOrStop(List<ScheduleNodeType> nodeTypes, Options ops,
            ScheduleHelpGenerator hp, boolean isRequire, boolean haveAllOpDesc)
            throws ScheduleToolsException {
        addTypeOptionForCreate(null, nodeTypes, ops, hp, isRequire, haveAllOpDesc, true);
    }

    public static void addTypeOptionForCreate(Map<String, ScheduleNodeRequiredParamGroup> nodeProperties,
            List<ScheduleNodeType> nodeTypes, Options ops, ScheduleHelpGenerator hp, boolean isRequire,
            boolean haveAllOpDesc, boolean withTypeNum) throws ScheduleToolsException {
        StringBuilder typeOptDesc = new StringBuilder();
        typeOptDesc.append("specify node type, arg:");
        // first 拼接 [ 1 | 2 | 3 | 21 | 20 ]
        StringBuilder first = new StringBuilder();
        // second 拼接 0:all, 1:service-center, 2:gateway, ...
        StringBuilder second = new StringBuilder();

        first.append("[");
        // all
        if (haveAllOpDesc) {
            if (withTypeNum) {
                first.append(" 0 |");
                second.append("0:all,").append(System.lineSeparator());
            }
            else {
                second.append("all, ").append(System.lineSeparator());
            }
        }
        // 拼接 nodeType
        for (ScheduleNodeType nodeType : nodeTypes) {
            first.append(String.format(" %s |", nodeType.getType()));
            if (withTypeNum) {
                second.append(String.format("%s:%s, ", nodeType.getType(), nodeType.getName()));
            }
            else {
                second.append(String.format("%s, ", nodeType.getName()));
            }
            if (nodeProperties != null && nodeProperties.size() > 0) {
                second.append("required properties:").append(System.lineSeparator());
                ScheduleNodeRequiredParamGroup ScheduleNodeRequiredParamGroup = nodeProperties.get(nodeType.getType());
                if (ScheduleNodeRequiredParamGroup != null) {
                    for (String str : ScheduleNodeRequiredParamGroup.getExample()) {
                        second.append("\t");
                        second.append(str);
                        second.append(System.lineSeparator());
                    }
                }
            }
            else {
                second.append(System.lineSeparator());
            }
        }

        // 去掉末尾的 "|"
        first.deleteCharAt(first.length() - 1);
        first.append("],").append(System.lineSeparator());

        // 合并
        if (withTypeNum) {
            typeOptDesc.append(first);
        }
        else {
            typeOptDesc.append(System.lineSeparator());
        }
        typeOptDesc.append(second);
        Option op = hp.createOpt(OPT_SHORT_NODE_TYPE, OPT_LONG_NODE_TYPE, typeOptDesc.toString(),
                isRequire, true, false);
        ops.addOption(op);
    }

    // type without typeNum
    public static void addTypeOptionForStartOrStopWithOutTypeNum(List<ScheduleNodeType> nodeTypes,
            Options ops, ScheduleHelpGenerator hp) throws ScheduleToolsException {
        addTypeOptionForCreate(null, nodeTypes, ops, hp, false, true, false);
    }

    public static CommandLine parseArgs(String[] args, Options options, boolean stopAtNonOption)
            throws ScheduleToolsException {
        CommandLine commandLine;
        CommandLineParser parser = new DefaultParser();
        try {
            commandLine = parser.parse(options, args, stopAtNonOption);
            return commandLine;
        }
        catch (ParseException e) {
            logger.error("Invalid arg", e);
            throw new ScheduleToolsException(e.getMessage(), ScheduleBaseExitCode.INVALID_ARG);
        }
    }

    public static String getPidCommandByjarName(String jarNamePrefix) {
        return "ps -eo pid,cmd | grep " + jarNamePrefix + " | grep -w -v grep | grep -w -v nohup";
    }

    public static int getPidFromPsResult(String psResult) {
        String trim = psResult.trim();
        String pidStr = trim.substring(0, trim.indexOf(" "));
        return Integer.valueOf(pidStr);
    }

    public static CommandLine parseArgs(String[] args, Options options) throws ScheduleToolsException {
        return parseArgs(args, options, false);
    }

    public static boolean isContainHelpArg(String[] args) {
        for (String arg : args) {
            if (arg.equals("-" + OPT_SHORT_HELP) || arg.equals("--" + OPT_LONG_HELP)
                    || arg.equals(OPT_LONG_HELP)) {
                return true;
            }
        }
        return false;
    }

    public static boolean isNeedPrintVersion(String[] args) {
        if (args.length == 1) {
            if (args[0].equals("-" + OPT_SHORT_VER) || args[0].equals("--" + OPT_LONG_VER)) {
                return true;
            }
        }
        return false;
    }

    public static int getTimeout(CommandLine commandLine, String timeoutOptName)
            throws ScheduleToolsException {
        int shortestTimeout = 5; // 5s
        String timeOutStr = commandLine.getOptionValue(timeoutOptName);
        int timeout = ScheduleCommon.convertStrToInt(timeOutStr);
        if (timeout < shortestTimeout) {
            timeout = shortestTimeout;
        }
        return timeout * 1000;
    }

    private static String readRetypeOptionValue(String optionName) throws ScheduleToolsException {
        String password1 = readOptionValue(optionName);
        System.out.print("retype " + optionName + " value: ");
        String password2 = readPasswdFromStdIn();
        if (!password1.equals(password2)) {
            throw new ScheduleToolsException("passwords do not match", ScheduleBaseExitCode.INVALID_ARG);
        }
        return password1;
    }

    private static String readOptionValue(String optionName) throws ScheduleToolsException {
        System.out.print(optionName + " value: ");
        return readPasswdFromStdIn();
    }

    public static String readPasswdFromStdIn() {
        return new String(System.console().readPassword());
    }

}
