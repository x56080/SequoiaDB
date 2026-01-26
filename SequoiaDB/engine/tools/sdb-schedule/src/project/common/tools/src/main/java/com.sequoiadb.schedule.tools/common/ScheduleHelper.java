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

   Source File Name = ScheduleHelper.java

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
import com.sequoiadb.schedule.tools.element.ScheduleNodeType;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.FilenameFilter;
import java.io.InputStream;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.regex.Pattern;

public class ScheduleHelper {
    private static Logger logger = LoggerFactory.getLogger(ScheduleHelper.class);
    private static String pwd;
    public static String getPwd() {
        if (pwd == null) {
            File pwdFile = new File("");
            pwd = pwdFile.getAbsolutePath();

            return pwd;
        }
        return pwd;
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


    public static String getJarNameByType(final ScheduleNodeType type, String jarsPath)
            throws ScheduleToolsException {
        File jarsDir = new File(jarsPath);
        final String matchCondition = "^" + type.getJarNamePrefix() + "(.*)\\.jar$";
        FilenameFilter jarNameFilter = new FilenameFilter() {
            @Override
            public boolean accept(File dir, String name) {
                return Pattern.matches(matchCondition, name);
            }
        };
        File[] files = jarsDir.listFiles(jarNameFilter);
        if (files == null || files.length <= 0) {
            logger.error("Missing jar, jarPrefix:{}, path:{}", matchCondition, jarsPath);
            throw new ScheduleToolsException("Missing jar, jarPrefix:" + matchCondition + ",path:" + jarsPath,
                    ScheduleBaseExitCode.FILE_NOT_FIND);
        }
        if (files.length == 1) {
            return files[0].getName();
        }
        Comparator<File> versionComparator = new Comparator<File>() {
            @Override
            public int compare(File o1, File o2) {
                return o2.getName().compareTo(o1.getName());
            }
        };
        Arrays.sort(files, versionComparator);
        logger.info("Multiple jar in the path, jar:{}, path:{}", Arrays.toString(files), jarsPath);
        return files[0].getName();
    }

    public static String getJarPathByType(final ScheduleNodeType type, String servicePath)
            throws ScheduleToolsException {
        String jarPath = servicePath + File.separator + "jars";
        String jarName = ScheduleHelper.getJarNameByType(type, jarPath);

        return jarPath + File.separator + jarName;
    }
    
    public static String getAbsolutePathFromTool(String path) {
        return Paths.get(path).normalize().toString();
    }

    public static void configToolsLog(String logFile) throws ScheduleToolsException {
        InputStream is = ScheduleHelper.class.getClassLoader().getResourceAsStream(logFile);
        try {
            configToolsLog(is);
        }
        finally {
            try {
                is.close();
            }
            catch (Exception e) {
                logger.warn("close inputStream occur error", e);
            }
        }
    }

    public static List<String> getSdbUrlList(String sdbUrl) {
        List<String> retList = new ArrayList<>();
        String[] urlArr = sdbUrl.split(",");
        for (String url : urlArr) {
            retList.add(url);
        }
        return retList;
    }

    public static void configToolsLog(InputStream is) {
        try {
            LoggerContext lc = (LoggerContext) LoggerFactory.getILoggerFactory();
            JoranConfigurator configure = new JoranConfigurator();
            configure.setContext(lc);
            lc.reset();
            configure.doConfigure(is);
        }
        catch (JoranException e) {
            e.printStackTrace();
            logger.error("config logback failed", e);
        }
    }
}
