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

   Source File Name = ScheduleMgrWrapper.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core;

import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.ScheduleCommonTools;
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.common.ScheduleEntityTranslator;
import com.sequoiadb.schedule.core.elect.LeaderElect;
import com.sequoiadb.schedule.core.job.CleanJobInfo;
import com.sequoiadb.schedule.core.job.DataSwitchJobInfo;
import com.sequoiadb.schedule.core.job.SchJobCreateContext;
import com.sequoiadb.schedule.core.job.ScheduleMgr;
import com.sequoiadb.schedule.core.job.TransferJobInfo;
import com.sequoiadb.schedule.core.job.quartz.QuartzScheduleMgr;
import com.sequoiadb.schedule.core.job.ScheduleJobInfo;
import com.sequoiadb.schedule.dao.ScheduleDao;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.metasource.SdbMetaCursor;
import com.sequoiadb.schedule.model.ScheduleFullEntity;
import com.sequoiadb.schedule.model.ScheduleUserEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.quartz.SchedulerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Date;
import java.util.UUID;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.function.Function;

public class ScheduleMgrWrapper {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleMgrWrapper.class);
    private static ScheduleMgrWrapper instance = new ScheduleMgrWrapper();
    Lock scheduleLock = new ReentrantLock();
    private ScheduleMgr mgr;
    private ScheduleDao scheduleDao;
    private LeaderElect leaderElect;

    private ScheduleMgrWrapper() {
    }

    public static ScheduleMgrWrapper getInstance() {
        return instance;
    }

    public void init(ScheduleDao scheduleDao, LeaderElect leaderElect) {
        this.scheduleDao = scheduleDao;
        this.leaderElect = leaderElect;
    }

    public ScheduleFullEntity getSchedule(String scheduleId) throws Exception {
        ScheduleFullEntity info = scheduleDao.queryOne(scheduleId);
        if (null == info) {
            throw new ScheduleServerException(ScheduleServerError.RECORD_NOT_EXISTS,
                    "schedule is not exist:schedule_id=" + scheduleId);
        }

        return info;
    }

    public void deleteSchedule(String scheduleId) throws Exception {
        scheduleLock.lock();
        try {
            if (null == mgr) {
                throw new Exception("mgr is null");
            }

            scheduleDao.delete(scheduleId);
            deleteJobSilence(scheduleId);
            logger.info("delete schedule job success:id={}", scheduleId);
        }
        catch (Exception e) {
            logger.error("delete schedule failed:scheduleId={}", scheduleId);
            throw e;
        }
        finally {
            scheduleLock.unlock();
        }
    }

    private void deleteJobSilence(String jobId) {
        try {
            if (null == mgr) {
                return;
            }

            mgr.deleteJob(jobId);
        }
        catch (Exception e) {
            logger.warn("delete job failed:jobId={}", jobId, e);
        }
    }

    public ScheduleFullEntity createSchedule(ScheduleUserEntity userInfo) throws Exception {
        Date date = new Date();
        String id = UUID.randomUUID().toString();
        ScheduleFullEntity info = ScheduleEntityTranslator.FullInfo.fromUserInfo(userInfo, id,
                date.getTime());
        ScheduleJobInfo jobInfo = createJobInfo(info);

        scheduleLock.lock();
        try {
            if (null == mgr) {
                throw new Exception("mgr is null");
            }

            SchJobCreateContext context = mgr.prepareCreateJob(jobInfo);

            // 先入库，防止任务跑起来找不到记录
            scheduleDao.insert(info);

            if (info.isEnable()) {
                try {
                    mgr.createJob(context);
                }
                catch (Exception e) {
                    if (e instanceof SchedulerException) {
                        if (isTriggerExpireException(e.getMessage())) {
                            // 如果是因为cron过期抛出的异常，就禁用该调度任务，不需要 revote
                            logger.warn(
                                    "failed to create schedule job, schedule info's cron is overdue, this schedule job will not be executed, disable this schedule:{}",
                                    info, e);
                            disableScheduleSilence(info);
                            return info;
                        }
                    }
                    logger.error("failed to create schedule job, disable this schedule:id={}",
                            info.getId(), e);
                    revote();
                    return info;
                }
            }
            logger.info("create schedule job success:id={},enable={}", jobInfo.getId(),
                    info.isEnable());
            return info;
        }
        finally {
            scheduleLock.unlock();
        }
    }

    private void revote() {
        try {
            leaderElect.resignLeader();
        }
        catch (Exception e) {
            logger.error("revote failed", e);
            ScheduleCommonTools.exitProcess();
        }
    }

    public ScheduleFullEntity switchSchedule(String scheduleId, boolean enable) throws Exception {
        return updateScheduleInternal(scheduleId, oldFullInfo -> {
            oldFullInfo.setEnable(enable);
            return ScheduleEntityTranslator.FullInfo.updateInfo(oldFullInfo, scheduleId,
                    oldFullInfo.getCreateTime());
        });
    }

    public ScheduleFullEntity updateSchedule(String scheduleId, ScheduleUserEntity newInfo)
            throws Exception {
        return updateScheduleInternal(scheduleId, oldFullInfo -> ScheduleEntityTranslator.FullInfo
                .updateInfo(newInfo, scheduleId, oldFullInfo.getCreateTime()));
    }

    private ScheduleFullEntity updateScheduleInternal(String scheduleId,
            Function<ScheduleFullEntity, ScheduleFullEntity> updater) throws Exception {
        ScheduleJobInfo oldJobInfo = null;
        boolean isOldJobDeleted = false;
        boolean isNewJobCreated = false;
        scheduleLock.lock();
        try {
            if (mgr == null) {
                throw new Exception("mgr is null");
            }

            ScheduleFullEntity oldFullInfo = scheduleDao.queryOne(scheduleId);
            if (oldFullInfo == null) {
                throw new ScheduleServerException(ScheduleServerError.RECORD_NOT_EXISTS,
                        "schedule not exist: scheduleId=" + scheduleId);
            }

            // 使用传入的 lambda 定制化更新逻辑
            ScheduleFullEntity newFullInfo = updater.apply(oldFullInfo);
            newFullInfo.setUpdateTime(System.currentTimeMillis());

            oldJobInfo = mgr.getJobInfo(scheduleId);
            if (oldJobInfo != null) {
                mgr.deleteJob(scheduleId);
                isOldJobDeleted = true;
            }

            if (newFullInfo.isEnable()) {
                ScheduleJobInfo newJobInfo = createJobInfo(newFullInfo);
                mgr.createJob(mgr.prepareCreateJob(newJobInfo));
                isNewJobCreated = true;
            }

            scheduleDao.updateByScheduleId(scheduleId, newFullInfo);

            logger.info("update schedule job success: id={}", scheduleId);
            return newFullInfo;
        }
        catch (Exception e) {
            logger.error("update schedule failed: scheduleId={}", scheduleId, e);
            try {
                if (isNewJobCreated) {
                    mgr.deleteJob(scheduleId);
                }
                if (isOldJobDeleted) {
                    mgr.createJob(mgr.prepareCreateJob(oldJobInfo));
                }
            }
            catch (Exception e2) {
                logger.warn("restore schedule failed: oldJobInfo={}", oldJobInfo, e2);
                revote();
            }
            throw e;
        }
        finally {
            scheduleLock.unlock();
        }
    }

    public void start() throws Exception {
        scheduleLock.lock();
        try {
            if (null != mgr) {
                return;
            }

            mgr = new QuartzScheduleMgr();
            SdbMetaCursor cursor = null;
            try {
                cursor = scheduleDao.query(new BasicBSONObject());
                while (cursor.hasNext()) {
                    BSONObject obj = cursor.getNext();
                    restoreJob(obj);
                }
            }
            finally {
                if (null != cursor) {
                    cursor.close();
                }
            }

            mgr.start();
        }
        catch (Exception e) {
            clearWithoutLock();
            throw e;
        }
        finally {
            scheduleLock.unlock();
        }
    }

    private void clearWithoutLock() {
        if (null != mgr) {
            mgr.clear();
            mgr = null;
        }
    }

    public void clear() {
        scheduleLock.lock();
        try {
            clearWithoutLock();
        }
        finally {
            scheduleLock.unlock();
        }
    }

    private void restoreJob(BSONObject scheduleRecord) throws Exception {
        ScheduleFullEntity info;
        try {
            info = ScheduleEntityTranslator.FullInfo.fromBSONObject(scheduleRecord);
        }
        catch (Exception e) {
            logger.warn("unrecognized schedule record:{}", scheduleRecord, e);
            return;
        }

        if (!info.isEnable()) {
            return;
        }

        SchJobCreateContext context = null;
        try {
            ScheduleJobInfo jobInfo = createJobInfo(info);
            context = mgr.prepareCreateJob(jobInfo);
        }
        catch (Exception e) {
            logger.warn("schedule info contain some invalid arguments, disable this schedule:{}",
                    info, e);
            disableScheduleSilence(info);
            return;
        }
        try {
            mgr.createJob(context);
        }
        catch (SchedulerException e) {
            // job的cron表达式过期，job将会一直无法被创建执行，并且会抛出 trigger 过期的异常。
            // 导致调度服务切主时执行restoreJob时启这种调度任务，遇到createJob异常就revote
            // 最后所有的调度服务节点都无法当主,无法处理后续业务操作。
            if (isTriggerExpireException(e.getMessage())) {
                // createJob 时，如果是 trigger 过期的异常，则将该job对应的schedule禁用
                logger.warn(
                        "schedule info's cron is overdue, this schedule job will not be executed, disable this schedule:{}",
                        info, e);
                disableScheduleSilence(info);
                return;
            }
            throw e;
        }
    }

    private boolean isTriggerExpireException(String message) {
        return message.contains("Based on configured schedule, the given trigger")
                && message.contains("will never fire");
    }

    private void disableScheduleSilence(ScheduleFullEntity info) {
        try {
            BasicBSONObject newValue = new BasicBSONObject();
            newValue.put(FieldName.Schedule.FIELD_ENABLE, false);
            scheduleDao.updateByScheduleId(info.getId(), newValue);
        }
        catch (Exception e) {
            logger.warn("failed to disable schedule:id={}", info.getId(), e);
        }
    }

    private ScheduleJobInfo createJobInfo(ScheduleFullEntity info) throws Exception {
        ScheduleJobInfo jobInfo = null;
        if (info.getType().equals(ScheduleDefine.ScheduleType.TRANSFER)) {
            jobInfo = new TransferJobInfo(info.getId(), info.getType(), info.getCron(),
                    info.getContent());
        }
        else if (info.getType().equals(ScheduleDefine.ScheduleType.DATA_SWITCH)) {
            jobInfo = new DataSwitchJobInfo(info.getId(), info.getType(), info.getCron(),
                    info.getContent());
        }
        else if (info.getType().equals(ScheduleDefine.ScheduleType.CLEAN)) {
            jobInfo = new CleanJobInfo(info.getId(), info.getType(), info.getCron(),
                    info.getContent());
        }
        else {
            throw new IllegalArgumentException("not support schedule type:" + info.getType());
        }
        return jobInfo;
    }
}
