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

   Source File Name = HashSlotLock.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.common;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class HashSlotLock {

    private final Map<Integer, ReadWriteLock> lockMap;
    private final int lockSize;

    public HashSlotLock(int lockSize) {
        this.lockSize = lockSize;
        lockMap = new ConcurrentHashMap<Integer, ReadWriteLock>(lockSize);
    }

    public LockWrapper getReadLock(String key) {
        checkNotNull(key);
        int position = getHash(key) % lockSize;
        ReadWriteLock readWriteLock = getLock(position);
        return new LockWrapper(readWriteLock.readLock(), true);
    }

    public LockWrapper getWriteLock(String key) {
        checkNotNull(key);
        int position = getHash(key) % lockSize;
        ReadWriteLock readWriteLock = getLock(position);
        return new LockWrapper(readWriteLock.writeLock(), false);
    }

    private void checkNotNull(String key) {
        if (key == null) {
            throw new IllegalArgumentException("key can not be null");
        }
    }

    private ReadWriteLock getLock(int position) {
        ReadWriteLock readWriteLock = lockMap.get(position);
        if (readWriteLock == null) {
            synchronized (lockMap) {
                readWriteLock = lockMap.get(position);
                if (readWriteLock == null) {
                    readWriteLock = new ReentrantReadWriteLock();
                    lockMap.put(position, readWriteLock);
                }
            }
        }
        return readWriteLock;
    }

    private int getHash(String key) {
        return Math.abs(key.hashCode());
    }
}
