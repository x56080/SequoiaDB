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

   Source File Name = TestConfig.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test;

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

public class TestConfig {
    private static final String propertiesFileName = "test.properties";
    private static Properties properties;

    static {
        properties = new Properties();
        InputStream in = TestConfig.class.getClassLoader().getResourceAsStream(propertiesFileName);
        try {
            properties.load(in);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static final String singleHost = "single.host";
    private static final String singlePort = "single.port";
    private static final String singleUsername = "single.username";
    private static final String singlePassword = "single.password";
    private static final String singleGroup = "single.group";
    private static final String nodeHost = "node.host";
    private static final String nodePort = "node.port";

    private TestConfig() {
    }

    public static String getSingleHost() {
        return properties.getProperty(singleHost);
    }

    public static String getSinglePort() {
        return properties.getProperty(singlePort);
    }

    public static String getSingleUsername() {
        return properties.getProperty(singleUsername);
    }

    public static String getSinglePassword() {
        return properties.getProperty(singlePassword);
    }

    public static String getSingleGroup() {
        return properties.getProperty(singleGroup);
    }

    public static String getNodeHost() {
        return properties.getProperty(nodeHost);
    }

    public static int getNodePort() {
        String port = properties.getProperty(nodePort);
        if (port != null && !port.isEmpty()) {
            return Integer.valueOf(port);
        } else {
            return 0;
        }
    }
}