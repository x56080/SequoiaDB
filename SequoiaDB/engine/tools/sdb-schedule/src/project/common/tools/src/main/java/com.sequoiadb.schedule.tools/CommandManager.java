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

   Source File Name = CommandManager.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

package com.sequoiadb.schedule.tools;

import com.sequoiadb.schedule.tools.command.ScheduleHelpFullToolImpl;
import com.sequoiadb.schedule.tools.command.ScheduleHelpToolImpl;
import com.sequoiadb.schedule.tools.command.ScheduleTool;
import com.sequoiadb.schedule.tools.common.ScheduleCommon;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class CommandManager {
    private Logger logger = LoggerFactory.getLogger(CommandManager.class.getName());;
    private Map<String, ScheduleTool> tools;
    private String commandManagerName;

    // logger 自己定义就行
    public CommandManager(String commandManagerName) {
        this.commandManagerName = commandManagerName;
        initTool();
    }

    public void initTool() {
        tools = new HashMap<>();
        this.tools.put("help", new ScheduleHelpToolImpl(this));
        this.tools.put("helpfull", new ScheduleHelpFullToolImpl(this));
    }

    public void addTool(ScheduleTool tool) {
        this.tools.put(tool.getToolName(), tool);
    }

    public void execute(String[] args) {
        execute(args, true);
    }

    public void execute(String[] args, boolean withAdminLog) {
        if (args.length > 0) {
            ScheduleTool tool = null;
            try {
                tool = getInstanceByToolName(args[0], withAdminLog);
            }
            catch (Exception e) {
                logger.error("create  " + args[0] + " subcommand instance failed", e);
                System.err.println("create " + args[0] + " subcommand instance failed:" + e.getMessage());
                if (e instanceof ScheduleToolsException) {
                    System.exit(((ScheduleToolsException)e).getExitCode());
                }
                System.exit(ScheduleBaseExitCode.SYSTEM_ERROR);
            }

            if (tool != null) {
                String[] toolsArgs = Arrays.copyOfRange(args, 1, args.length);
                try {
                    this.checkHelpArgs(args);
                    tool.process(toolsArgs);
                    System.exit(ScheduleBaseExitCode.SUCCESS);
                }
                catch (ScheduleToolsException e) {
                    if (e.getExitCode() != ScheduleBaseExitCode.EMPTY_OUT) {
                        logAndPrintErr(args[0], e);
                    }
                    System.exit(e.getExitCode());
                }
                catch (Exception e) {
                    logAndPrintErr(args[0], e);
                    System.exit(ScheduleBaseExitCode.SYSTEM_ERROR);
                }
            }
            else {
                try {
                    checkHelpArgs(args);
                }
                catch (Exception e) {
                    logger.error("analyze args failed", e);
                    System.err.println("analyze args failed:" + e.getMessage());
                    if (e instanceof ScheduleToolsException) {
                        System.exit(((ScheduleToolsException)e).getExitCode());
                    }
                    System.exit(ScheduleBaseExitCode.SYSTEM_ERROR);
                }
                System.out.println("No such subcommand");
            }
        }
        System.out.println(this.getHelpMsg(false));
        System.exit(ScheduleBaseExitCode.INVALID_ARG);
    }

    private void logAndPrintErr(String arg, Exception e) {
        logger.error("process failed,subcommand:" + arg, e);
        System.err.println("process failed,subcommand:" + arg);
        if (e.getMessage() != null) {
            System.err.println("error message:" + e.getMessage());
        }
    }

    public void checkHelpArgs(String[] args) throws ScheduleToolsException {
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-h") || args[i].equals("--help")) {
                if (i < args.length - 1) {
                    printHelp(args[i + 1], false);
                }
                else {
                    System.out.println(this.getHelpMsg(false));
                    System.exit(ScheduleBaseExitCode.SUCCESS);
                }
            }
        }
    }

    public void printHelp(String arg, boolean isFullHelp) throws ScheduleToolsException {
        ScheduleTool tool = getInstanceByToolName(arg, false);
        if (tool != null) {
            tool.printHelp(isFullHelp);
            System.exit(ScheduleBaseExitCode.SUCCESS);
        }
        else {
            System.out.println("No such command");
            System.out.println(this.getHelpMsg(isFullHelp));
            System.exit(ScheduleBaseExitCode.INVALID_ARG);
        }
    }

    private ScheduleTool getInstanceByToolName(String toolName, boolean withAdminLog)
            throws ScheduleToolsException {
        ScheduleTool instance = null;
        if (withAdminLog) {
            ScheduleCommon.configToolsLog(ScheduleCommon.LOG_FILE_ADMIN);
        }
        instance = this.tools.get(toolName);
        return instance;
    }

    public String getHelpMsg(boolean isFullHelp) {
        String template = "usage: name <subcommand> [options] [args]" + "\r\n"
                + "Type 'name help [subcommand]' for help on a specific subcommand" + "\r\n"
                + "Type 'name --version / -v' to see the program version" + "\r\n"
                + "Available subcommands:" + "\r\n";
        template = template.replaceAll("name", this.commandManagerName);
        StringBuilder sb = new StringBuilder(template);
        Set<String> cmdNames = null;
        if (isFullHelp) {
            cmdNames = this.tools.keySet();
        }
        else {
            cmdNames = new HashSet<>();
            for (Map.Entry<String, ScheduleTool> entry : this.tools.entrySet()) {
                if (!entry.getValue().isHidden()) {
                    cmdNames.add(entry.getKey());
                }
            }
        }
        for (String cmdName : cmdNames) {
            String s = String.format("\t%s" + "\r\n", cmdName);
            sb.append(s);
        }

        return sb.toString();
    }

    protected Map<String, ScheduleTool> getTools() {
        return tools;
    }
}
