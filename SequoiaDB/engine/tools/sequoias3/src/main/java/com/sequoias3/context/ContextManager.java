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

   Source File Name = ContextManager.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.context;

import com.sequoias3.config.ContextConfig;
import com.sequoias3.config.ServiceInfo;
import com.sequoias3.dao.MetaDao;
import org.apache.commons.codec.binary.Hex;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import java.util.*;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

@Component
public class ContextManager {
    private static final Logger logger = LoggerFactory.getLogger(ContextManager.class);
    private Lock lock = new ReentrantLock();

    @Autowired
    MetaDao metaDao;

    @Autowired
    ContextConfig contextConfig;

    @Autowired
    ServiceInfo serviceInfo;

    private AtomicLong index = new AtomicLong(0);
    private Map<String, Context> contextMap = new HashMap<>();

    public Context create(long bucketId){
        long newIndex = this.index.getAndIncrement();
        StringBuilder buildIndex = new StringBuilder();
        buildIndex.append("B");
        buildIndex.append(bucketId);
        buildIndex.append("P");
        buildIndex.append(serviceInfo.getPort());
        buildIndex.append("H");
        buildIndex.append(serviceInfo.getHost());
        buildIndex.append("I");
        buildIndex.append(newIndex);
        buildIndex.append("E");
        String indexString = buildIndex.toString();
        String token = new String(Hex.encodeHex(indexString.getBytes()));
        lock.lock();
        try {
            Context context = new Context(token, bucketId);
            context.setLastModified(System.currentTimeMillis());
            contextMap.put(token, context);
            return context;
        }finally {
            lock.unlock();
        }
    }

    public void release(Context context){
        lock.lock();
        try {
            if (null != context) {
                contextMap.remove(context.getToken());
            }
        }finally {
            lock.unlock();
        }
    }

    public Context get(String continueToken){
        lock.lock();
        try {
            Context context = contextMap.get(continueToken);
            if (context != null) {
                context.setLastModified(System.currentTimeMillis());
            }
            return context;
        }finally {
            lock.unlock();
        }
    }

    public void cleanExpiredContext(){
        logger.debug("before scan. contextMap size:"+contextMap.size());
        long contextMaxLife = contextConfig.getLifecycle() * 60 * 1000;

        lock.lock();
        try {
            long nowTime = System.currentTimeMillis();
            Iterator<Map.Entry<String, Context>> it = contextMap.entrySet().iterator();
            while (it.hasNext()) {
                Map.Entry<String, Context> entry = it.next();
                long diff = nowTime - entry.getValue().getLastModified();
                if (diff > contextMaxLife) {
                    logger.info("release context. context{}", entry.getValue());
                    it.remove();
                }
            }
        }finally {
            lock.unlock();
        }

        logger.debug("after scan. contextMap size:"+contextMap.size());
    }
}
