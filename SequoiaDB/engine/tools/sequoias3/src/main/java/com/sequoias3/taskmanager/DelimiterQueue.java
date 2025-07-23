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

   Source File Name = DelimiterQueue.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.taskmanager;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

@Component("DelimiterQueue")
public class DelimiterQueue {
    private static final Logger logger = LoggerFactory.getLogger(DelimiterQueue.class);
    private Lock taskLock = new ReentrantLock();

    private LinkedHashSet<String> bucketList = new LinkedHashSet();

    public void addBucketName(String bucketName){
        taskLock.lock();
        try{
            bucketList.add(bucketName);
            logger.info("add deleting delimiter. bucketName={}, after queue.size={}", bucketName, bucketList.size());
        }finally {
            taskLock.unlock();
        }
    }

    public String getBucketName(){
        taskLock.lock();
        try {
            if (bucketList.size() > 0) {
                Iterator it = bucketList.iterator();
                String bucketName;
                if (it.hasNext()){
                    bucketName = (String)(it.next());
                    it.remove();
                }else {
                    return null;
                }

//                String bucketName = bucketList.(0);
//                bucketList.remove(0);
                logger.info("get deleting delimiter. bucketName={}, after queue.size={}", bucketName, bucketList.size());
                return bucketName;
            } else {
                return null;
            }
        }finally {
            taskLock.unlock();
        }
    }
}
