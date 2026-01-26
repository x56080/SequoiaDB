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

   Source File Name = QuartzScheduleJob.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core.job.quartz;

import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.core.job.ScheduleJobInfo;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import org.quartz.Job;
import org.quartz.JobDataMap;
import org.quartz.JobDetail;
import org.quartz.JobExecutionContext;
import org.quartz.JobExecutionException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class QuartzScheduleJob implements Job {

    private static final Logger logger = LoggerFactory.getLogger(QuartzScheduleJob.class);

    public QuartzScheduleJob() {
    }

    @Override
    public void execute(JobExecutionContext context) throws JobExecutionException {
        JobDetail jobDetail = context.getJobDetail();
        JobDataMap dataMap = jobDetail.getJobDataMap();

        ScheduleJobInfo info = null;
        try {
            info = (ScheduleJobInfo) dataMap.get(FieldName.Schedule.FIELD_SCH_INFO);
            if (info == null) {
                throw new ScheduleServerException(ScheduleServerError.INTERNAL_ERROR,
                        "schedule info not found in datamap:key="
                                + FieldName.Schedule.FIELD_SCH_INFO + ", datamap=" + dataMap);
            }
            execute(info, context);
        }
        catch (Exception e) {
            logger.warn("execute job failed:info={}", info, e);
            throw new JobExecutionException("execute job failed", e);
        }
    }

    public abstract void execute(ScheduleJobInfo info, JobExecutionContext context)
            throws ScheduleServerException, JobExecutionException;
}
