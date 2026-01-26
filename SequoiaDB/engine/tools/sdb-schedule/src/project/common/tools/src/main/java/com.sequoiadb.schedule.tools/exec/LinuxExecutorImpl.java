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

   Source File Name = LinuxExecutorImpl.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.tools.exec;

import com.sequoiadb.schedule.tools.common.ScheduleCommandUtil;
import com.sequoiadb.schedule.tools.common.ScheduleCommon;
import com.sequoiadb.schedule.tools.common.ScheduleHelpGenerator;
import com.sequoiadb.schedule.tools.common.ScheduleToolsDefine;
import com.sequoiadb.schedule.tools.element.ScheduleNodeProcessInfo;
import com.sequoiadb.schedule.tools.element.ScheduleNodeStatus;
import com.sequoiadb.schedule.tools.element.ScheduleNodeType;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.apache.commons.io.IOUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;

public class LinuxExecutorImpl implements Executor {
    private static final Logger logger = LoggerFactory.getLogger(LinuxExecutorImpl.class);

    @Override
    public void startNode(String jarPath, String springConfigLocation, String loggingConfig,
            String errorLogPath, String options, String workingDir) throws ScheduleToolsException {
        if(ScheduleCommon.isNeedBackup(errorLogPath)){
            ScheduleCommon.backupErrorOut(errorLogPath);
        }
        ScheduleCommon.printStartInfo(errorLogPath);
        String cmd = " nohup java " + options + " -jar '" + jarPath + "' --spring.config.location="
                + springConfigLocation + " --logging.config=" + loggingConfig + " >> " + errorLogPath
                + " 2>&1 &";
        logger.info("starting node by exec cmd(/bin/sh -c \" " + cmd + "\")");
        try {
            execShell(cmd, workingDir);
        }
        catch (ScheduleToolsException e) {
            throw new ScheduleToolsException("start node failed, error:" + e.getMessage(),
                    e.getExitCode(), e);
        }
    }

    @Override
    public void stopNode(int pid, boolean isForce) throws ScheduleToolsException {
        String killCmd;
        if (isForce) {
            logger.info("stopping kill -9 " + pid);
            killCmd = "kill -9 " + pid;
        }
        else {
            logger.info("stopping kill -15 " + pid);
            killCmd = "kill -15 " + pid;
        }
        Process ps = exec(killCmd);
        int rc;
        try {
            rc = ps.waitFor();
            // rc == 1 : pid not found
            if (rc == 0) {
                return;
            }
            String errorMsg = getConsoleOutput(ps);
            if (rc == 1 && errorMsg.contains("No such process")) {
                return;
            }
            else {
                logger.error(
                        "stop node failed,cmd:/bin/sh -c \"" + killCmd + "\",errorMsg:" + errorMsg);
                if (errorMsg.contains("Operation not permitted")) {
                    throw new ScheduleToolsException("failed to stop,pid:" + pid + ",error:" + errorMsg,
                            ScheduleBaseExitCode.PERMISSION_ERROR);
                }
                else {
                    throw new ScheduleToolsException("failed to stop,pid:" + pid + ",error:" + errorMsg,
                            ScheduleBaseExitCode.SHELL_EXEC_ERROR);
                }
            }
        }
        catch (InterruptedException e) {
            logger.error("wait cmd return occur error,cmd:/bin/sh -c \"" + killCmd + "\"", e);
            throw new ScheduleToolsException("wait cmd return occur error,cmd:/bin/sh -c \"" + killCmd
                    + "\",error:" + e.getMessage(), ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        catch (IOException e) {
            logger.error("get cmd output failed,cmd:/bin/sh -c \"" + killCmd + "\"", e);
            throw new ScheduleToolsException(
                    "get cmd std failed,cmd:/bin/sh -c \"" + killCmd + "\",error:" + e.getMessage(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        catch (Exception e) {
            logger.error("exec cmd occur error,cmd:/bin/sh -c \"" + killCmd + "\"", e);
            throw new ScheduleToolsException(
                    "get cmd std failed,cmd:/bin/sh -c \"" + killCmd + "\",error:" + e.getMessage(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        finally {
            ps.destroy();
        }
    }
    private String getConfPathStr(String psTrimedStr, String contentServerIdentify)
            throws ScheduleToolsException {
        int argsIdx = psTrimedStr.indexOf(" ", psTrimedStr.indexOf(contentServerIdentify));
        if (argsIdx < 0) {
            return null;
        }

        Options ops = new Options();
        ScheduleHelpGenerator hp = new ScheduleHelpGenerator();
        ops.addOption(hp.createOpt(null, ScheduleToolsDefine.PROPERTIES.APPLICATION_PROPERTIES_LOCATION,
                "", true, true, false));
        String[] args = psTrimedStr.substring(argsIdx).trim().split(" ");
        String confFile = null;
        for (String arg : args) {
            String[] tmpArray = new String[] { arg };
            CommandLine cl = ScheduleCommandUtil.parseArgs(tmpArray, ops, true);
            if (cl.hasOption(ScheduleToolsDefine.PROPERTIES.APPLICATION_PROPERTIES_LOCATION)) {
                confFile = cl
                        .getOptionValue(ScheduleToolsDefine.PROPERTIES.APPLICATION_PROPERTIES_LOCATION);
                break;
            }
        }

        if (null == confFile) {
            return null;
        }

        File f = new File(confFile);
        if (f.isFile()) {
            f = f.getParentFile();
        }

        return f.getAbsolutePath();
    }

    @Override
    public ScheduleNodeStatus getNodeStatus(ScheduleNodeType nodeType) throws ScheduleToolsException {
        ScheduleNodeStatus psRes = new ScheduleNodeStatus();

        _getNodeStatus(nodeType, psRes);

        return psRes;
    }

    @Override
    public void execShell(String cmd) throws ScheduleToolsException {
        execShell(cmd, null);
    }

    public String execShell(String cmd, String workingDir) throws ScheduleToolsException {
        Process ps = exec(cmd, workingDir);

        try {
            int rc = ps.waitFor();
            if (rc != 0) {
                String errorMsg = getConsoleOutput(ps);
                logger.error("exec cmd failed,cmd:/bin/sh -c \"" + cmd + "\",errorMsg:" + errorMsg);
                throw new ScheduleToolsException("exec cmd failed,error:" + errorMsg,
                        ScheduleBaseExitCode.SHELL_EXEC_ERROR);
            }
            return getConsoleOutput(ps);
        }
        catch (InterruptedException e) {
            logger.error("wait cmd return occur error,cmd:/bin/sh -c \"" + cmd + "\"", e);
            throw new ScheduleToolsException("wait cmd return occur interrupted exception,cmd:" + cmd
                    + ",error:" + e.getMessage(), ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        catch (IOException e) {
            logger.error("get cmd output failed,cmd:/bin/sh -c \"" + cmd + "\"", e);
            throw new ScheduleToolsException(
                    "get cmd std failed,cmd:/bin/sh -c \"" + cmd + "\",error:" + e.getMessage(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        catch (Exception e) {
            logger.error("get cmd output failed,cmd:/bin/sh -c \"" + cmd + "\"", e);
            ScheduleCommon.throwToolException("get cmd std failed,cmd:/bin/sh -c \"" + cmd + "\"", e);
            // impossible
            return null;
        }
        finally {
            ps.destroy();
        }
    }

    public void _getNodeStatus(ScheduleNodeType nodeType, ScheduleNodeStatus res) throws ScheduleToolsException {
        String[] psCmd = { "/bin/sh", "-c",
                ScheduleCommandUtil.getPidCommandByjarName(nodeType.getJarNamePrefix()) };
        Process ps = null;
        int rc;
        try {
            ps = Runtime.getRuntime().exec(psCmd);
            rc = ps.waitFor();
            if (rc == 1) {
                // no ps result
                return;
            }
            if (rc != 0) {
                String errMsg = getConsoleOutput(ps);
                logger.error("failed to exec cmd:" + cmd2Str(psCmd) + ",error:" + errMsg);
                throw new ScheduleToolsException(
                        "failed to exec cmd:" + cmd2Str(psCmd) + ",error:" + errMsg,
                        ScheduleBaseExitCode.SHELL_EXEC_ERROR);
            }

        }
        catch (IOException e) {
            if (ps != null) {
                ps.destroy();
            }
            logger.error("exec cmd occur io error,cmd" + cmd2Str(psCmd), e);
            throw new ScheduleToolsException(
                    "Failed to exec:" + cmd2Str(psCmd) + ",error:" + e.getMessage(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        catch (InterruptedException e) {
            if (ps != null) {
                ps.destroy();
            }
            logger.error("wait cmd return occur error,cmd:" + cmd2Str(psCmd), e);
            throw new ScheduleToolsException(
                    "Failed to exec:" + cmd2Str(psCmd) + ",error:" + e.getMessage(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        catch (Exception e) {
            if (ps != null) {
                ps.destroy();
            }
            logger.error("exec cmd occur error,cmd:" + cmd2Str(psCmd), e);
            throw new ScheduleToolsException(
                    "Failed to exec:" + cmd2Str(psCmd) + ",error:" + e.getMessage(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }

        BufferedReader bfr = null;
        try {
            bfr = new BufferedReader(new InputStreamReader(ps.getInputStream()));
            while (true) {
                String lineSrc = bfr.readLine();
                if (lineSrc == null) {
                    break;
                }
                parseLine(lineSrc, res, nodeType);
            }
        }
        catch (IOException e) {
            logger.error("Failed to access ps std out", e);
            throw new ScheduleToolsException("Failed to access ps std out:" + e.getMessage(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        catch (Exception e) {
            logger.error("Failed to access ps std out", e);
            throw new ScheduleToolsException("Failed to access ps std out:" + e.getMessage(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        finally {
            closeStream(bfr);
            ps.destroy();
        }
    }

    private void parseLine(String lineSrc, ScheduleNodeStatus psRes, ScheduleNodeType nodeType) {
        try {
            int pid = ScheduleCommandUtil.getPidFromPsResult(lineSrc);
            String confPath = getConfPathStr(lineSrc.trim(), nodeType.getJarNamePrefix());
            if (null != confPath) {
                if (confPath.startsWith(".")) {
                    File f = new File("");
                    confPath = f.getAbsolutePath() + confPath.substring(1);
                }

                psRes.addNode(new ScheduleNodeProcessInfo(pid, confPath, nodeType));
            }
        }
        catch (Exception e) {
            logger.warn("failed to parse,ignore a line in the ps std out:line=" + lineSrc, e);
        }
    }

    private String getConsoleOutput(Process ps) throws IOException {
        BufferedReader ebfr = null;
        BufferedReader stdbfr = null;
        String out = new String();
        try {
            stdbfr = new BufferedReader(new InputStreamReader(ps.getInputStream()));
            out += readReader(stdbfr);
            ebfr = new BufferedReader(new InputStreamReader(ps.getErrorStream()));
            out += readReader(ebfr);
            if (out.endsWith("\n")) {
                out = out.substring(0, out.length() - 1);
            }
        }
        finally {
            closeStream(stdbfr);
            closeStream(ebfr);
        }
        return out;
    }

    private String readReader(BufferedReader ebfr) throws IOException {
        return IOUtils.toString(ebfr);
    }

    private Process exec(String command) throws ScheduleToolsException {
        return exec(command, null);
    }

    private Process exec(String command, String workingDir) throws ScheduleToolsException {
        Process ps;
        String[] cmd = new String[3];
        cmd[0] = "/bin/sh";
        cmd[1] = "-c";
        cmd[2] = command;
        try {
            File dir = null;
            if (workingDir != null) {
                dir = new File(workingDir);
            }
            ps = Runtime.getRuntime().exec(cmd, null, dir);
            return ps;
        }
        catch (IOException e) {
            logger.error("exec cmd occur io error,cmd" + cmd2Str(cmd), e);
            throw new ScheduleToolsException("exec cmd occur io error,cmd" + cmd2Str(cmd),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        catch (Exception e) {
            logger.error("exec cmd occur error,cmd" + cmd2Str(cmd), e);
            throw new ScheduleToolsException("exec cmd occur error,cmd" + cmd2Str(cmd),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }

    }

    private void closeStream(Reader r) {
        if (r != null) {
            try {
                r.close();
            }
            catch (Exception e) {
                logger.warn("close reader occur error", e);
            }
        }
    }

    private String cmd2Str(String[] cmdArray) {
        String tmp = "";
        for (String str : cmdArray) {
            tmp = tmp + str + " ";
        }
        return tmp;
    }

}
