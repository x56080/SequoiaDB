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

   Source File Name = ScheduleBackgroundJob.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core.task;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.TimerTask;

public abstract class ScheduleBackgroundJob extends TimerTask {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleBackgroundJob.class);
    /**
     * get the job's type, define in ServiceDefine.job
     *
     * @return the job's type
     */
    public abstract int getType();

    /**
     * get the job's name
     *
     * @return the job's name
     */
    public abstract String getName();

    /**
     * get the job's period time in milliseconds between successive job
     * executions. <=0, just run once; >0, run forever in period.
     *
     * @return the period time(ms)
     */
    public abstract long getPeriod();

    public abstract void _run();

    @Override
    public final void run() {
        try {
            _run();
        }
        catch (Throwable e) {
            logger.error("background job execution failed", e);
        }
    }

    public boolean retryOnThreadPoolReject() {
        return false;
    }

    public long waitingTimeOnReject() {
        // ms, 小于0表示使用默认配置
        return -1;
    }
}
