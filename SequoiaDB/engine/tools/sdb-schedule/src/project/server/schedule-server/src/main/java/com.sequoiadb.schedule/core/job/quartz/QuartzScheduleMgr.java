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

   Source File Name = QuartzScheduleMgr.java

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
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.core.job.SchJobCreateContext;
import com.sequoiadb.schedule.core.job.ScheduleJobInfo;
import com.sequoiadb.schedule.core.job.ScheduleMgr;
import org.quartz.CronScheduleBuilder;
import org.quartz.JobDataMap;
import org.quartz.JobDetail;
import org.quartz.JobKey;
import org.quartz.Scheduler;
import org.quartz.Trigger;
import org.quartz.TriggerBuilder;
import org.quartz.impl.StdSchedulerFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.Map;

public class QuartzScheduleMgr implements ScheduleMgr {
    private static final Logger logger = LoggerFactory.getLogger(QuartzScheduleMgr.class);
    private static final String CONTEXT_KEY_TRIGGER = "QuartzSchTrigger";
    private static final String CONTEXT_KEY_DETAIL = "QuartzSchDetail";

    Scheduler sch;
    private Map<String, ScheduleJobInfo> createdJobInfos = new HashMap<>();

    public QuartzScheduleMgr() throws Exception {
        sch = StdSchedulerFactory.getDefaultScheduler();
    }

    @Override
    public SchJobCreateContext prepareCreateJob(ScheduleJobInfo info) throws Exception {
        JobDetail detail = null;
        TriggerBuilder<Trigger> tgBuilder = TriggerBuilder.newTrigger();
        JobKey jobKey = QuartzScheduleTools.createJobKey(info.getId());
        JobDataMap dataMap = new JobDataMap();
        dataMap.put(FieldName.Schedule.FIELD_SCH_INFO, info);
        if (info.getType().equals(ScheduleDefine.ScheduleType.TRANSFER)) {
            detail = QuartzScheduleTools.createJobDetail(QuartzTransferJob.class, dataMap, jobKey);
            tgBuilder.forJob(detail).withSchedule(CronScheduleBuilder.cronSchedule(info.getCron()));
        }
        else if (info.getType().equals(ScheduleDefine.ScheduleType.DATA_SWITCH)) {
            detail = QuartzScheduleTools.createJobDetail(QuartzDataSwitchJob.class, dataMap, jobKey);
            tgBuilder.forJob(detail).withSchedule(CronScheduleBuilder.cronSchedule(info.getCron()));
        }
        else if (info.getType().equals(ScheduleDefine.ScheduleType.CLEAN)) {
            detail = QuartzScheduleTools.createJobDetail(QuartzCleanJob.class, dataMap, jobKey);
            tgBuilder.forJob(detail).withSchedule(CronScheduleBuilder.cronSchedule(info.getCron()));
        }
        else {
            throw new IllegalArgumentException("not support job type:" + info.getType());
        }
        Trigger trigger = tgBuilder.build();
        SchJobCreateContext contex = new SchJobCreateContext(info);
        contex.set(CONTEXT_KEY_TRIGGER, trigger);
        contex.set(CONTEXT_KEY_DETAIL, detail);
        return contex;
    }

    @Override
    public void createJob(SchJobCreateContext contex) throws Exception {
        Trigger trigger = contex.get(CONTEXT_KEY_TRIGGER, Trigger.class);
        JobDetail detail = contex.get(CONTEXT_KEY_DETAIL, JobDetail.class);
        sch.scheduleJob(detail, trigger);
        createdJobInfos.put(contex.getJobInfo().getId(), contex.getJobInfo());
    }

    @Override
    public void start() throws Exception {
        sch.start();
    }

    @Override
    public void clear() {
        _clear();
        shutdown();
    }

    @Override
    public ScheduleJobInfo getJobInfo(String id) {
        return createdJobInfos.get(id);
    }

    private void _clear() {
        try {
            sch.clear();
        }
        catch (Exception e) {
            logger.warn("clear schedule failed", e);
        }
        finally {
            createdJobInfos.clear();
        }
    }

    @Override
    public void deleteJob(String id) throws Exception {
        sch.deleteJob(QuartzScheduleTools.createJobKey(id));
        ScheduleJobInfo jobInfo = createdJobInfos.remove(id);
        if (jobInfo == null) {
            return;
        }
    }

    private void shutdown() {
        try {
            sch.shutdown();
        }
        catch (Exception e) {
            logger.warn("shutdown schedule failed", e);
        }
    }
}
