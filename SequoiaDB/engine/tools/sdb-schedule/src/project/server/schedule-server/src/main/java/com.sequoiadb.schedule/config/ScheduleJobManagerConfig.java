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

   Source File Name = ScheduleJobManagerConfig.java

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

import com.sequoiadb.schedule.core.task.ScheduleJobManagerRefresher;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.cloud.context.config.annotation.RefreshScope;

import javax.annotation.PostConstruct;

@ConfigurationProperties(prefix = "schedule.jobmanager.threadpool")
@RefreshScope
public class ScheduleJobManagerConfig {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleJobManagerConfig.class);
    private int scheduleTaskThreadPoolCoreSize = 20;
    private int scheduleTaskThreadPoolMaxSize = 20;
    private int scheduleTaskThreadPoolQueueSize = 100;
    private static boolean isInitialized = false;

    @PostConstruct
    public void onRefresh() {
        if (scheduleTaskThreadPoolCoreSize > scheduleTaskThreadPoolMaxSize) {
            logger.warn("coreSize:{} is greater than maxSize:{}, update coreSize to:{}",
                    scheduleTaskThreadPoolCoreSize, scheduleTaskThreadPoolQueueSize,
                    scheduleTaskThreadPoolQueueSize);
            scheduleTaskThreadPoolCoreSize = scheduleTaskThreadPoolMaxSize;
        }
        if (isInitialized) {
            ScheduleJobManagerRefresher.refreshThreadPoolConfig(scheduleTaskThreadPoolCoreSize,
                    scheduleTaskThreadPoolMaxSize);
        }
        else {
            isInitialized = true;
        }
    }

    public int getScheduleTaskThreadPoolCoreSize() {
        return scheduleTaskThreadPoolCoreSize;
    }

    public void setScheduleTaskThreadPoolCoreSize(int scheduleTaskThreadPoolCoreSize) {
        if (scheduleTaskThreadPoolCoreSize <= 0 || scheduleTaskThreadPoolCoreSize > 100) {
            logger.warn("Invalid scheduleTaskThreadPoolCoreSize value: "
                    + scheduleTaskThreadPoolCoreSize + ", set to default value: "
                    + this.scheduleTaskThreadPoolCoreSize);
            return;
        }
        this.scheduleTaskThreadPoolCoreSize = scheduleTaskThreadPoolCoreSize;
    }

    public void setScheduleTaskThreadPoolMaxSize(int scheduleTaskThreadPoolMaxSize) {
        if (scheduleTaskThreadPoolMaxSize <= 0 || scheduleTaskThreadPoolMaxSize > 100) {
            logger.warn(
                    "Invalid scheduleTaskThreadPoolMaxSize value: " + scheduleTaskThreadPoolMaxSize
                            + ", set to default value: " + this.scheduleTaskThreadPoolMaxSize);
            return;
        }
        this.scheduleTaskThreadPoolMaxSize = scheduleTaskThreadPoolMaxSize;
    }

    public int getScheduleTaskThreadPoolMaxSize() {
        return scheduleTaskThreadPoolMaxSize;
    }

    public int getScheduleTaskThreadPoolQueueSize() {
        return scheduleTaskThreadPoolQueueSize;
    }

    public void setScheduleTaskThreadPoolQueueSize(int scheduleTaskThreadPoolQueueSize) {
        if (scheduleTaskThreadPoolQueueSize <= 0 || scheduleTaskThreadPoolQueueSize > 100) {
            logger.warn("Invalid scheduleTaskThreadPoolQueueSize value: "
                    + scheduleTaskThreadPoolQueueSize + ", set to default value: "
                    + this.scheduleTaskThreadPoolQueueSize);
            return;
        }
        this.scheduleTaskThreadPoolQueueSize = scheduleTaskThreadPoolQueueSize;
    }
}
