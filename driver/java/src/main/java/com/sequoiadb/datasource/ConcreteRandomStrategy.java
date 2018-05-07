/**
 * Copyright (C) 2018 SequoiaDB Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
package com.sequoiadb.datasource;

import java.util.Random;

class ConcreteRandomStrategy extends AbstractStrategy {
    private Random _rand = new Random(47);

    @Override
    public String getAddress() {
        String addr = null;
        _addrLock.lock();
        try {
            if (_addrs.size() >= 1) {
                addr = _addrs.get(_rand.nextInt(_addrs.size()));
            }
        } finally {
            _addrLock.unlock();
        }
        return addr;
    }

}
