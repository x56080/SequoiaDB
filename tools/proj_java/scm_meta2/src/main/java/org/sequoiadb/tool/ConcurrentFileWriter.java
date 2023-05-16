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

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.concurrent.locks.ReentrantLock;

public class ConcurrentFileWriter {
    private static final Logger logger = LoggerFactory.getLogger(ConcurrentFileWriter.class);
    private String filePath;
    private StringBuilder buffer;
    private ReentrantLock lock;
    private int writeThreshold;
    private PrintWriter printWriter;

    public ConcurrentFileWriter(String filePath, int bufferSize, int writeThreshold) {
        this.filePath = filePath;
        this.buffer = new StringBuilder(bufferSize);
        this.lock = new ReentrantLock();
        this.writeThreshold = writeThreshold;
        openFile();
    }

    private void openFile() {
        try {
            FileWriter fileWriter = new FileWriter(filePath, true);
            printWriter = new PrintWriter(fileWriter);
        } catch (IOException e) {
            String errMsg = "Failed to open file: " + filePath;
            logger.error(errMsg + ", " + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        }
    }

    public void writeToFile(String content) {
        try {
            lock.lock();
            buffer.append(content).append("\n");
            if (buffer.length() >= writeThreshold) {
                flushToFile();
            }
        } finally {
            lock.unlock();
        }
    }

    public void flushToFile() {
        try {
            lock.lock();
            if (buffer.length() == 0) {
                return; // 缓冲区为空，无需刷盘
            }

            printWriter.print(buffer.toString());
            printWriter.flush();
            buffer.setLength(0); // 清空缓冲区
        } finally {
            lock.unlock();
        }
    }

    public void closeFile() {
        try {
            lock.lock();
            flushToFile();
            printWriter.close();
        } finally {
            lock.unlock();
        }
    }


}
