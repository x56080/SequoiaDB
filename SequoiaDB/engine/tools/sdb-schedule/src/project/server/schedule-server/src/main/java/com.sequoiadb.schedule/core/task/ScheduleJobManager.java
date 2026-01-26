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

   Source File Name = ScheduleJobManager.java

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

import com.sequoiadb.schedule.common.ScheduleThreadFactory;
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.config.ScheduleJobManagerConfig;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleSystemException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.context.properties.EnableConfigurationProperties;
import org.springframework.stereotype.Component;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

@Component
@EnableConfigurationProperties(ScheduleJobManagerConfig.class)
public class ScheduleJobManager {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleJobManager.class);
    private final ThreadPoolExecutor scheduleTaskThreadPool;
    private static volatile ScheduleJobManager jobManager;

    private Timer timer = TimerFactory.createTimer(2);

    @Autowired
    public ScheduleJobManager(ScheduleJobManagerConfig jobManagerConfig) {
        this.scheduleTaskThreadPool = new ThreadPoolExecutor(
                jobManagerConfig.getScheduleTaskThreadPoolCoreSize(),
                jobManagerConfig.getScheduleTaskThreadPoolMaxSize(), 60, TimeUnit.SECONDS,
                new ArrayBlockingQueue<>(jobManagerConfig.getScheduleTaskThreadPoolQueueSize()),
                new ScheduleThreadFactory("scheduleTaskThreadPool"),
                new ScheduleTaskRejectHandler());
        logger.info(
                "scheduleTaskThreadPool initialized, coreSize={}, maxSize={}, queueSize={}, rejectedHandler={}",
                scheduleTaskThreadPool.getCorePoolSize(),
                scheduleTaskThreadPool.getMaximumPoolSize(),
                scheduleTaskThreadPool.getQueue().remainingCapacity(),
                scheduleTaskThreadPool.getRejectedExecutionHandler());

        ScheduleJobManager.jobManager = this;
    }

    public static ScheduleJobManager getInstance() {
        checkState();
        return jobManager;
    }

    public int getCoreThreadSize() {
        checkState();
        return scheduleTaskThreadPool.getCorePoolSize();
    }

    public int getMaxThreadSize() {
        checkState();
        return scheduleTaskThreadPool.getMaximumPoolSize();
    }

    public void updateThreadPoolConfig(int coreThreadSize, int maxThreadSize) {
        checkState();
        // 需要先设置核心线程，再设置最大线程，否则当新的最大线程比旧的核心线程小时就会报错
        scheduleTaskThreadPool.setCorePoolSize(coreThreadSize);
        scheduleTaskThreadPool.setMaximumPoolSize(maxThreadSize);
    }

    public void executeScheduleTask(Runnable task) throws ScheduleServerException {
        checkState();
        try {
            scheduleTaskThreadPool.execute(task);
        }
        catch (Exception e) {
            if (task instanceof ScheduleTaskBase) {
                String taskId = ((ScheduleTaskBase) task).getTaskId();
                throw new ScheduleSystemException("failed to execute task, taskId=" + taskId, e);
            }
            throw new ScheduleSystemException("failed to execute task", e);
        }

    }

    static class ScheduleTaskRejectHandler implements RejectedExecutionHandler {

        @Override
        public void rejectedExecution(Runnable r, ThreadPoolExecutor executor) {
            if (executor.isShutdown()) {
                return;
            }
            BlockingQueue<Runnable> queue = executor.getQueue();
            if (!queue.offer(r)) {
                throw new RejectedExecutionException("Failed to add task to queue, queue is full");
            }
        }
    }

    /*
     * @param delay(ms)
     */
    public void schedule(final ScheduleBackgroundJob task, long delay)
            throws ScheduleServerException {
        checkState();
        try {
            TimerTask timerTask = new TimerTask() {
                @Override
                public void run() {
                    task.run();
                }
            };
            long period = task.getPeriod();
            if (period > 0) {
                timer.schedule(timerTask, delay, period);
            }
            else {
                if (delay > 0) {
                    timer.schedule(timerTask, delay);
                }
                else {
                    // todo
                    // executeShortTimeTask(timerTask);
                }
            }

            logger.debug("start BackgroundJob success:type=" + task.getType() + ",name="
                    + task.getName() + ",period=" + task.getPeriod());
        }
        catch (Exception e) {
            throw new ScheduleServerException(ScheduleServerError.INTERNAL_ERROR,
                    "start background job failed:type=" + task.getType() + ",name="
                            + task.getName(),
                    e);
        }
    }

    private static void checkState() {
        if (jobManager == null) {
            throw new RuntimeException("ScheduleJobManager is not initialized");
        }
    }
}
