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

   Source File Name = TimerThreadPoolImpl.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.common.timer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Date;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

public class TimerThreadPoolImpl implements Timer {
    private static final Logger logger = LoggerFactory.getLogger(TimerThreadPoolImpl.class);
    private ScheduledExecutorService innerExecutorService = null;

    public TimerThreadPoolImpl() {
        innerExecutorService = Executors.newScheduledThreadPool(1);
    }

    public TimerThreadPoolImpl(int coreSize) {
        innerExecutorService = Executors.newScheduledThreadPool(coreSize);
    }

    public TimerThreadPoolImpl(final String name) {
        innerExecutorService = Executors.newScheduledThreadPool(1, new ThreadFactory() {
            @Override
            public Thread newThread(Runnable r) {
                return new Thread(null, r, name);
            }
        });
    }

    public TimerThreadPoolImpl(final String name, final boolean isDaemon) {
        innerExecutorService = Executors.newScheduledThreadPool(1, new ThreadFactory() {
            @Override
            public Thread newThread(Runnable r) {
                Thread t = new Thread(null, r, name);
                t.setDaemon(isDaemon);
                return t;
            }
        });
    }

    @Override
    public void schedule(TimerTask task, long delayInMillionSecond, long periodInMillisecond) {
        ScheduledFuture<?> future = innerExecutorService.scheduleAtFixedRate(task,
                delayInMillionSecond, periodInMillisecond, TimeUnit.MILLISECONDS);
        task.setFuture(future);
    }

    @Override
    public void schedule(TimerTask task, long delayInMillionSecond) {
        ScheduledFuture<?> future = innerExecutorService.schedule(task, delayInMillionSecond,
                TimeUnit.MILLISECONDS);
        task.setFuture(future);
    }

    @Override
    public void cancel() {
        try {
            innerExecutorService.shutdown();
        }
        catch (Exception e) {
            logger.warn("failed shutdown timer", e);
        }
    }

    @Override
    public boolean cancelAndAwaitTermination(long timeout) throws InterruptedException {
        innerExecutorService.shutdown();
        return innerExecutorService.awaitTermination(timeout, TimeUnit.MILLISECONDS);
    }

    @Override
    public void schedule(TimerTask task, Date firstTime, long periodInMillionSecond) {
        long now = System.currentTimeMillis();
        long delay = firstTime.getTime() - now;
        if (delay < 0) {
            delay = 0;
        }
        schedule(task, delay, periodInMillionSecond);
    }

    @Override
    public void schedule(TimerTask task, Date firstTime) {
        long now = System.currentTimeMillis();
        long delay = firstTime.getTime() - now;
        if (delay < 0) {
            delay = 0;
        }
        schedule(task, delay);
    }
}
