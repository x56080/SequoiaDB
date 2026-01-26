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

   Source File Name = PropertiesUtil.java

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

import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Map;
import java.util.Properties;

public class PropertiesUtil {
    private static final Logger logger = LoggerFactory.getLogger(PropertiesUtil.class);
    // sysconf.properties
    public final static String SAMPLE_VALUE_SCHEDULE_LOG_PATH = "LOG_PATH_VALUE";

    public static Properties loadProperties(File file) throws ScheduleToolsException {

        FileInputStream is;
        try {
            is = new FileInputStream(file);
        }
        catch (FileNotFoundException e) {
            logger.error("file not found:" + file.getPath(), e);
            throw new ScheduleToolsException("file not found:" + file.getPath(),
                    ScheduleBaseExitCode.FILE_NOT_FIND);

        }
        Properties prop = new Properties();
        try {
            prop.load(is);
        }
        catch (IOException e) {
            logger.error("failed to load file:" + file.getParent(), e);
            throw new ScheduleToolsException(
                    "failed to load file:" + file.getParent() + ",errormsg:" + e.getMessage(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        finally {
            try {
                is.close();
            }
            catch (Exception e) {
                logger.warn("close inputStream error,file:" + file.getPath(), e);
            }
        }

        return prop;
    }

    public static Properties loadProperties(String filePath) throws ScheduleToolsException {
        File toolLog4j = new File(filePath);
        return loadProperties(toolLog4j);
    }

    public static void writeProperties(Map<String, String> items, String propPath)
            throws ScheduleToolsException {
        File file = new File(propPath);
        if (!file.exists()) {
            ScheduleCommon.createFile(propPath);
        }
        Properties prop = new Properties();
        for (String key : items.keySet()) {
            prop.setProperty(key, items.get(key));
        }
        PrintWriter pw = null;
        try {
            pw = new PrintWriter(file);
            prop.store(pw, "UTF-8");
        }
        catch (FileNotFoundException e) {
            throw new ScheduleToolsException("Failed to write " + file.getName(),
                    ScheduleBaseExitCode.FILE_NOT_FIND, e);
        }
        catch (IOException e) {
            throw new ScheduleToolsException("Failed to write " + file.getName(), ScheduleBaseExitCode.SYSTEM_ERROR,
                    e);
        }
        catch (Exception e) {
            throw new ScheduleToolsException("Failed to write " + file.getName(),
                    ScheduleBaseExitCode.SYSTEM_ERROR, e);
        }
        finally {
            if (pw != null) {
                try {
                    pw.close();
                }
                catch (Exception e) {
                    logger.warn("Failed to close resource:{}", pw, e);
                }
            }
        }
    }
}
