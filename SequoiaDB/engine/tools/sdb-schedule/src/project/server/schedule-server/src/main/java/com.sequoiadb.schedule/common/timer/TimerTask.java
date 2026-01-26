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

   Source File Name = TimerTask.java

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

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Future;

public abstract class TimerTask implements Runnable {
    private final Logger logger = LoggerFactory.getLogger(TimerTask.class);
    private final CountDownLatch setFuture = new CountDownLatch(1);
    private volatile Future<?> myFuture = null;

    public void cancel() {
        while (true) {
            try {
                setFuture.await();
                break;
            }
            catch (Exception e) {
                if (myFuture != null) {
                    break;
                }
                logger.warn("failed to wait for task future, try again", e);
            }
        }
        myFuture.cancel(false);
    }

    final void setFuture(Future<?> myFuture) {
        if (this.myFuture != null) {
            throw new RuntimeException("task already scheduled");
        }
        this.myFuture = myFuture;
        setFuture.countDown();
    }
}
