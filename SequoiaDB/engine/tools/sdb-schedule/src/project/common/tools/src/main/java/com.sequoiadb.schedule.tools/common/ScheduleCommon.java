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

   Source File Name = ScheduleCommon.java

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

import ch.qos.logback.classic.LoggerContext;
import ch.qos.logback.classic.joran.JoranConfigurator;
import ch.qos.logback.core.joran.spi.JoranException;
import ch.qos.logback.core.rolling.FixedWindowRollingPolicy;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.sql.Timestamp;

public class ScheduleCommon {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleCommon.class.getName());
    public static final String LOG_FILE_ADMIN = "logback_admin.xml";
    public static final String APPLICATION_PROPERTIES = "application.properties";
    public static final String LOGCONF_NAME = "logback.xml";
    public static final String SAMPLE_SYS_CONF_NAME = "sample.application.properties";
    public static final String SCHEDULE_LOG_DIR_NAME = "schedule-server";
    public static final String SAMPLE_LOG_CONF_NAME = "sample.logback.xml";

    public static final long  MAX_ERROR_FILE_SIZE = 104857600 ; //100MB

    public static String getScheduleConfAbsolutePath() {
        File f = new File("./");
        return f.getAbsolutePath() + File.separator + "conf" + File.separator + "schedule-server"
                + File.separator;
    }

    public static CommandLine parseArgs(String[] args, Options options) throws ScheduleToolsException {
        return parseArgs(args, options, false);
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

    public static void createFile(String filePath) throws ScheduleToolsException {
        File file = new File(filePath);
        if (!ScheduleCommon.isFileExists(file.getParent())) {
            try {
                Files.createDirectories(Paths.get(file.getParent()));
            }
            catch (SecurityException e) {
                logger.error("Failed to create dir:" + file.getParent(), e);
                throw new ScheduleToolsException("Failed to create dir:" + file.getParent()
                        + ",permisson error:" + e.getMessage(), ScheduleBaseExitCode.PERMISSION_ERROR);
            }
            catch (IOException e) {
                logger.error("Failed to create dir:" + file.getParent(), e);
                throw new ScheduleToolsException(
                        "Failed to create dir:" + file.getParent() + ",io error:" + e.getMessage(),
                        ScheduleBaseExitCode.SYSTEM_ERROR);
            }
            catch (Exception e) {
                logger.error("Failed to create dir:" + file.getParentFile().toString(), e);
                throw new ScheduleToolsException("Failed to create dir:"
                        + file.getParentFile().toString() + ",error:" + e.getMessage(),
                        ScheduleBaseExitCode.SYSTEM_ERROR);
            }
            // setFileOwnerAndGroup(file.getParent());
        }
        try {
            if (!file.createNewFile()) {
                logger.error("Failed to create file:" + file.getAbsolutePath()
                        + ",caused by file is already exist");
                throw new ScheduleToolsException(
                        "Failed to create file:" + file.getAbsolutePath()
                                + ",caused by file is already exist",
                        ScheduleBaseExitCode.FILE_ALREADY_EXIST);
            }
            // setFileOwnerAndGroup(file.getPath());
        }
        catch (IOException e) {
            logger.error("Faile to create file:" + filePath, e);
            throw new ScheduleToolsException("Failed to create file,io exception:" + filePath
                    + ",errorMsg:" + e.getMessage(), ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        catch (SecurityException e) {
            logger.error("Faile to create file:" + filePath, e);
            throw new ScheduleToolsException("Failed to create file,permission error:" + filePath
                    + ",errorMsg:" + e.getMessage(), ScheduleBaseExitCode.SYSTEM_ERROR);
        }
    }

    public static void throwToolException(String msg, Exception e) throws ScheduleToolsException {
        if (e instanceof ScheduleToolsException) {
            throw (ScheduleToolsException) e;
        }
        throw new ScheduleToolsException(msg + ":error=" + e.getMessage(), ScheduleBaseExitCode.SYSTEM_ERROR, e);
    }


    public static void printStartInfo(String errorLogPath) throws ScheduleToolsException {
        long timeMillis = System.currentTimeMillis();
        Timestamp timestamp = new Timestamp(timeMillis);
        String msg = "["+timestamp.toString()+"][com.sequoiadb.schedule.tools.common.ScheduleCommon][INFO ]: starting node";
        String[] cmd = new String[3];
        cmd[0] = "/bin/sh";
        cmd[1] = "-c";
        cmd[2] = "echo "+ msg +" >> "+ errorLogPath;
        try {
            Runtime.getRuntime().exec(cmd);
        }
        catch (IOException e) {
            throw new ScheduleToolsException("Exec cmd occur io error", ScheduleBaseExitCode.SHELL_EXEC_ERROR, e);
        }
        catch (Exception e) {
            throw new ScheduleToolsException("Exec cmd occur error", ScheduleBaseExitCode.SHELL_EXEC_ERROR, e);
        }
    }

    public static void backupErrorOut(String errorLogPath) throws ScheduleToolsException{
        File errorLogFile = new File(errorLogPath);
        String destLogFileName = errorLogFile.getParent() + File.separator + "error." + 1 + ".out";
        if (isFileExists(errorLogPath)) {
            File destLogFile = new File(destLogFileName);
            if (isFileExists(destLogFileName)) {
                if (!destLogFile.delete()) {
                    throw new ScheduleToolsException("Unable to delete " + destLogFileName, ScheduleBaseExitCode.FILE_DELETE_ERROR);
                }
                else {
                    errorLogFile.renameTo(destLogFile);
                }
            }
            else {
                errorLogFile.renameTo(destLogFile);
            }
        }
    }

    public static boolean isFileExists(String filePath) throws ScheduleToolsException {
        if (filePath == null) {
            return false;
        }
        File f = new File(filePath);
        try {
            if (f.exists()) {
                return true;
            }
            else {
                return false;
            }
        }
        catch (SecurityException e) {
            logger.error("Could not determine the existence of file:" + filePath, e);
            throw new ScheduleToolsException("Could not determine the existence of file:" + filePath,
                    ScheduleBaseExitCode.PERMISSION_ERROR);
        }
    }

    public static boolean isNeedBackup(String errorLogPath) throws ScheduleToolsException {
        if (!isFileExists(errorLogPath)) {
            return false;
        }
        File errorLogFile = new File(errorLogPath);
        if (errorLogFile.length() < MAX_ERROR_FILE_SIZE) {
            return false;
        }
        return true;
    }

    public static boolean isLinux() {
        String os = System.getProperties().getProperty("os.name").toLowerCase();
        if (os.contains("linux")) {
            return true;
        }
        else {
            return false;
        }
    }

    public static void createDir(String dirPath) throws ScheduleToolsException {
        File file = new File(dirPath);
        try {
            if (!file.exists() && file.mkdirs() != true) {
                logger.error("Faile to create dir:" + file.toString());
                throw new ScheduleToolsException("Faile to create dir:" + file.toString(),
                        ScheduleBaseExitCode.SYSTEM_ERROR);
            }
        }
        catch (SecurityException e) {
            logger.error("Failed to create dir:" + file.toString(), e);
            throw new ScheduleToolsException("Failed to create dir:" + file.toString()
                    + ",permisson error:" + e.getMessage(), ScheduleBaseExitCode.PERMISSION_ERROR);
        }
    }
    
    public static String getStopLogPath() {
        String serviceInstallPath = getServiceInstallPath();
        return new File(serviceInstallPath + File.separator + "log" + File.separator + "stop.log")
                .getPath();
    }

    public static void sleep(long millis) {
        try {
            Thread.sleep(millis);
        }
        catch (InterruptedException e) {
            // ignore
            logger.warn("sleep occur execption,millis:" + millis, e);
        }
    }

    public static String getStartLogPath() {
        String serviceInstallPath = getServiceInstallPath();
        return new File(serviceInstallPath + File.separator + "log" + File.separator + "start.log")
                .getPath();
    }

    public static String getServiceInstallPath() {
        String jarFilePath = ScheduleCommon.class.getProtectionDomain().getCodeSource().getLocation()
                .getPath();
        return new File(jarFilePath).getParentFile().getParent();
    }

    public static int convertStrToInt(String str) throws ScheduleToolsException {
        int port;
        try {
            port = Integer.valueOf(str);
            return port;
        }
        catch (Exception e) {
            logger.error("Can't convert " + str + " to integer", e);
            throw new ScheduleToolsException("Can't convert " + str + " to integer",
                    ScheduleBaseExitCode.INVALID_ARG);
        }
    }
    
    
    public static void printSpace(int count) {
        for (int i = 0; i < count; i++) {
            System.out.print(" ");
        }
    }

    public static void configToolsLog(String logFile) throws ScheduleToolsException {
        InputStream is = ScheduleCommon.class.getClassLoader().getResourceAsStream(logFile);
        try {
            LoggerContext lc = (LoggerContext) LoggerFactory.getILoggerFactory();
            JoranConfigurator configure = new JoranConfigurator();
            configure.setContext(lc);
            FixedWindowRollingPolicy f;
            lc.reset();
            configure.doConfigure(is);
        }
        catch (JoranException e) {
            e.printStackTrace();
            logger.error("config logback failed", e);
        }
        finally {
            try {
                is.close();
            }
            catch (Exception e) {
                logger.warn("close inpustream occur error", e);
            }
        }
    }
}
