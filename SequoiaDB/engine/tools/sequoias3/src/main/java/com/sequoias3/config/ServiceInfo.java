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

   Source File Name = ServiceInfo.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.config;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.boot.context.embedded.EmbeddedServletContainerInitializedEvent;
import org.springframework.context.ApplicationListener;
import org.springframework.stereotype.Component;

import java.lang.management.ManagementFactory;
import java.lang.management.RuntimeMXBean;
import java.net.InetAddress;
import java.net.UnknownHostException;

@Component
public class ServiceInfo implements ApplicationListener<EmbeddedServletContainerInitializedEvent> {
    private static final Logger logger = LoggerFactory.getLogger(ServiceInfo.class);
    private int port;
    private String host;
    private int pid;

    @Override
    public void onApplicationEvent(EmbeddedServletContainerInitializedEvent event){
        if (event != null){
            this.port = event.getEmbeddedServletContainer().getPort();
            RuntimeMXBean runtimeMXBean = ManagementFactory.getRuntimeMXBean();
            String name = runtimeMXBean.getName();
            this.pid = Integer.valueOf(name.substring(0, name.indexOf("@")));
        }
    }

    public ServiceInfo(){
        try {
            this.host = InetAddress.getLocalHost().getHostAddress();
        }catch (UnknownHostException e){
            logger.error("get server host failed. e", e);
        }
    }


    public int getPort(){
        return this.port;
    }

    public String getHost(){
        return this.host;
    }

    public int getPid() {
        return pid;
    }
}
