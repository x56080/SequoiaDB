/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.flink.util;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.TimeUnit;

public class RetryUtil {

    private static final Logger LOG = LoggerFactory.getLogger(RetryUtil.class);

    public static <T> T retryWhenRuntimeException(RetryContent<T> content, long maxRetryTimes, long duration) {
        int times = 0;
        while (times < maxRetryTimes) {
            try {
                return content.retry();
            } catch (RuntimeException ex) {
                ++times;
                try {
                    TimeUnit.MILLISECONDS.sleep(duration);
                } catch (InterruptedException ignored) {}

                LOG.info("{}, retry {} time(s), duration {} ms.", ex.getMessage(), times, duration);
            }
        }
        return null;
    }

    @FunctionalInterface
    public interface RetryContent<T> {
        T retry();
    }

}
