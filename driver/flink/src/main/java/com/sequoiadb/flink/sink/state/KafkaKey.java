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

package com.sequoiadb.flink.sink.state;

import java.util.Objects;

public class KafkaKey {

    private final String topic;
    private final Integer partition;

    public KafkaKey(String topic, Integer partition) {
        this.topic = topic;
        this.partition = partition;
    }

    @Override
    public int hashCode() {
        return Objects.hash(topic, partition);
    }

    @Override
    public boolean equals(Object anObj) {
        if (this == anObj) {
            return true;
        }
        if (anObj == null || getClass() != anObj.getClass()) {
            return false;
        }

        KafkaKey that = (KafkaKey) anObj;
        return Objects.equals(topic, that.topic) &&
               Objects.equals(partition, that.partition);
    }

    public String getTopic() {
        return topic;
    }

    public Integer getPartition() {
        return partition;
    }

}
