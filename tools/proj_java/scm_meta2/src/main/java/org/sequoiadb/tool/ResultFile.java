/**
 * Copyright (C) 2023 SequoiaDB Inc.
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
package org.sequoiadb.tool;

import org.bson.types.ObjectId;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

class ResultFile {
    private ConcurrentFileWriter fileWriter;

    public ResultFile(String filePath) {
        this.fileWriter = new ConcurrentFileWriter(filePath, Common.CACHE_SIZE_512KB, Common.CACHE_SIZE_512KB);
        init();
    }

    private void init() {
        // 写入开始工作的时间
        LocalDateTime now = LocalDateTime.now();
        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
        String currentTime = now.format(formatter);
        fileWriter.writeToFile("# Begin at " + currentTime);
    }

    public void close() {
        flush();
        fileWriter.closeFile();
    }

    public void flush() {
        fileWriter.flushToFile();
    }

    public void write(ObjectId oid) {
        fileWriter.writeToFile(oid.toString());
    }
}