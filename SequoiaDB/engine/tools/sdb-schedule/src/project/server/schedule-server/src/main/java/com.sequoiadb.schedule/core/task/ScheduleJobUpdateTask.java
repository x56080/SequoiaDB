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

   Source File Name = ScheduleJobUpdateTask.java

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

import com.sequoiadb.schedule.common.ScheduleDefine;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class ScheduleJobUpdateTask extends ScheduleBackgroundJob {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleJobUpdateTask.class);
    ReadWriteLock rwLock = new ReentrantReadWriteLock();
    Map<String, TaskUpdator> updatorMap = new HashMap<>();

    @Override
    public int getType() {
        return ScheduleDefine.Job.JOB_TYPE_UPDATE_TASK_STATUS;
    }

    @Override
    public String getName() {
        return "JOB_TYPE_UPDATE_TASK_STATUS";
    }

    @Override
    public long getPeriod() {
        // 5 minutes
        return 5 * 60 * 1000;
    }

    @Override
    public void _run() {
        Map<String, TaskUpdator> tmpMap = new HashMap<>();
        Lock r = rwLock.readLock();
        r.lock();
        try {
            tmpMap.putAll(updatorMap);
        }
        finally {
            r.unlock();
        }

        for (TaskUpdator updator : tmpMap.values()) {
            try {
                logger.debug("redo update:taskId=" + updator.getTaskId());
                updator.doUpdate();
                removeUpdator(updator.getTaskId());
            }
            catch (Exception e) {
                logger.error("updator task status failed:taskId=" + updator.getTaskId(), e);
            }
        }
    }

    private void removeUpdator(String taskId) {
        Lock w = rwLock.writeLock();
        w.lock();
        try {
            updatorMap.remove(taskId);
        }
        finally {
            w.unlock();
        }
    }

    public void addUpdator(TaskUpdator updator) {
        Lock w = rwLock.writeLock();
        w.lock();
        try {
            updatorMap.put(updator.getTaskId(), updator);
        }
        finally {
            w.unlock();
        }
    }
}
