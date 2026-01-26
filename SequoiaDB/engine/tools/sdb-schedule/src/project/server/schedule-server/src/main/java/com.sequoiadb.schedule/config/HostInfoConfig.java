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

   Source File Name = HostInfoConfig.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Configuration;

import javax.annotation.PostConstruct;
import java.net.InetAddress;
import java.net.UnknownHostException;

@Configuration
public class HostInfoConfig {

    @Value("${server.port}")
    private int serverPort;

    private String hostname;

    private String ipAddress;

    @PostConstruct
    public void init() throws UnknownHostException {
        hostname = InetAddress.getLocalHost().getHostName();
        ipAddress = InetAddress.getLocalHost().getHostAddress();
    }

    public int getServerPort() {
        return serverPort;
    }

    public String getHostname() {
        return hostname;
    }

    public String getIpAddress() {
        return ipAddress;
    }

    @Override
    public String toString() {
        return "HostInfoConfig{" + "serverPort=" + serverPort + ", hostname='" + hostname + '\''
                + ", ipAddress='" + ipAddress + '\'' + '}';
    }

    public String getHostAndPort() {
        return hostname + ":" + serverPort;
    }
}
