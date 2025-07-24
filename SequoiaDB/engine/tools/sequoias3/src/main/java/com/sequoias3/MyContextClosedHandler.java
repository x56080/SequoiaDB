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

   Source File Name = MyContextClosedHandler.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3;

import com.sequoias3.config.ServiceInfo;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.ApplicationListener;
import org.springframework.context.event.ContextClosedEvent;
import org.springframework.scheduling.concurrent.ThreadPoolTaskScheduler;
import org.springframework.stereotype.Component;

import java.io.File;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;

@Component
public class MyContextClosedHandler implements ApplicationListener<ContextClosedEvent> {
    private static final Logger logger = LoggerFactory.getLogger(MyContextClosedHandler.class);

    @Autowired
    @Qualifier("myThreadPoolScheduler")
    private ThreadPoolTaskScheduler myThreadPoolScheduler;

    @Autowired
    ServiceInfo serviceInfo;

    @Autowired
    SystemStatus systemStatus;

    @Override
    public void onApplicationEvent(ContextClosedEvent contextClosedEvent) {
        logger.info("SequoiaS3Application is stop with PID " + serviceInfo.getPid());
        systemStatus.exitSystemStatus();
        shutDownAndAwaitTermination(myThreadPoolScheduler.getScheduledExecutor());
        cleanTmpfile();
    }

    private void shutDownAndAwaitTermination(ExecutorService pool){
        try {
            pool.shutdown();
            if (!pool.awaitTermination(5, TimeUnit.SECONDS)){
                pool.shutdownNow();
                if (!pool.awaitTermination(5, TimeUnit.SECONDS)){
                    System.err.println("Pool did not terminate in 10s.");
                }
            }
        }catch (InterruptedException e){
            pool.shutdownNow();
            Thread.currentThread().interrupt();
        }
    }

    private void cleanTmpfile(){
        try {
            int processID = serviceInfo.getPid();
            String fileName = processID + ".pid";
            File file = new File(fileName);
            if (!file.exists()) {
                return;
            }
            file.delete();
        }catch (Exception e){
            logger.error("delete /tmp/s3pid.txt failed. e:"+e.getMessage());
        }
    }
}
