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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.locks.ReentrantLock;

class ResultFile {
    private final String FILE_MISS_RECORD = "miss_record";
    private final String FILE_EMPTY_SITE = "empty_site";
    private final String FILE_UPDATE_SUCC = "update_succ";
    private final String FILE_UPDATE_FAIL = "update_fail";

    private final int CACHE_SIZE_512KB = 512 * 1024;
    private final int CACHE_SIZE_1MB = 1 * 1024 * 1024;
    private final int CACHE_SIZE_2MB = 2 * 1024 * 1024;

    private String collectionName;
    private ConcurrentFileWriter missRecFileWriter;
    private ConcurrentFileWriter emptySiteFileWriter;
    private ConcurrentFileWriter updateSuccFileWriter;
    private ConcurrentFileWriter updateFailFileWriter;

    public ResultFile(String collectionName) {
        this.collectionName = collectionName;
        this.missRecFileWriter = new ConcurrentFileWriter(collectionName + "." + FILE_MISS_RECORD,
                CACHE_SIZE_512KB, CACHE_SIZE_512KB);
        this.emptySiteFileWriter = new ConcurrentFileWriter(collectionName + "." + FILE_EMPTY_SITE,
                CACHE_SIZE_512KB, CACHE_SIZE_512KB);
        this.updateSuccFileWriter = new ConcurrentFileWriter(collectionName + "." + FILE_UPDATE_SUCC,
                CACHE_SIZE_2MB, CACHE_SIZE_2MB);
        this.updateFailFileWriter = new ConcurrentFileWriter(collectionName + "." + FILE_UPDATE_FAIL,
                CACHE_SIZE_2MB, CACHE_SIZE_2MB);
        init();
    }

    private void init() {
        missRecFileWriter.openFile();
        emptySiteFileWriter.openFile();
        updateSuccFileWriter.openFile();
        updateFailFileWriter.openFile();
        // 写入开始工作的时间
        LocalDateTime now = LocalDateTime.now();
        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
        String currentTime = now.format(formatter);
        missRecFileWriter.writeToFile("# Begin at " + currentTime);
        emptySiteFileWriter.writeToFile("# Begin at " + currentTime);
        updateSuccFileWriter.writeToFile("# Begin at " + currentTime);
        updateFailFileWriter.writeToFile("# Begin at " + currentTime);
    }

    public void close() {
        missRecFileWriter.closeFile();
        emptySiteFileWriter.closeFile();
        updateSuccFileWriter.closeFile();
        updateFailFileWriter.closeFile();
    }

    public void flush() {
        missRecFileWriter.flushToFile();
        emptySiteFileWriter.flushToFile();
        updateSuccFileWriter.flushToFile();
        updateFailFileWriter.flushToFile();
    }

    public String getCollectionName() {
        return collectionName;
    }

    public ConcurrentFileWriter getMissRecFileWriter() {
        return missRecFileWriter;
    }

    public ConcurrentFileWriter getEmptySiteFileWriter() {
        return emptySiteFileWriter;
    }

    public ConcurrentFileWriter getUpdateSuccFileWriter() {
        return updateSuccFileWriter;
    }

    public ConcurrentFileWriter getUpdateFailFileWriter() {
        return updateFailFileWriter;
    }
}

public class FileMgr {
    private static final Logger logger = LoggerFactory.getLogger(FileMgr.class);
    HashMap<String, ResultFile> fileMap;
    ReentrantLock lock;

    public FileMgr() {
        fileMap = new HashMap<>();
        lock = new ReentrantLock();
    }

    public void registerCL(String fullName) {
        try {
            lock.lock();
            if (!fileMap.containsKey(fullName)) {
                ResultFile resultFile = new ResultFile(fullName);
                fileMap.put(fullName, resultFile);
            }
        } finally {
            lock.unlock();
        }
    }

    public void close() {
        try {
            lock.lock();
            for (Map.Entry<String, ResultFile> entry : fileMap.entrySet()) {
                entry.getValue().close();
            }
        } finally {
            lock.unlock();
        }
    }
    public void close(String fullName) {
        ResultFile resultFile;
        try {
            lock.lock();
            resultFile = fileMap.get(fullName);
        } finally {
            lock.unlock();
        }
        resultFile.close();
    }

    public void flush(String fullName) {
        ResultFile resultFile;
        try {
            lock.lock();
            resultFile = fileMap.get(fullName);
        } finally {
            lock.unlock();
        }
        resultFile.flush();
    }

    public void writeMissRecord(String fullName, ObjectId oid) {
        ConcurrentFileWriter fileWriter;
        try {
            lock.lock();
            fileWriter = fileMap.get(fullName).getMissRecFileWriter();
        } finally {
            lock.unlock();
        }
        fileWriter.writeToFile(oid.toString());
    }

    public void writeEmptySite(String fullName, ObjectId oid) {
        ConcurrentFileWriter fileWriter;
        try {
            lock.lock();
            fileWriter = fileMap.get(fullName).getEmptySiteFileWriter();
        } finally {
            lock.unlock();
        }
        fileWriter.writeToFile(oid.toString());
    }

    public void writeUpdateSucc(String fullName, ObjectId oid) {
        ConcurrentFileWriter fileWriter;
        try {
            lock.lock();
            fileWriter = fileMap.get(fullName).getUpdateSuccFileWriter();
        } finally {
            lock.unlock();
        }
        fileWriter.writeToFile(oid.toString());
    }

    public void writeUpdateFail(String fullName, ObjectId oid) {
        ConcurrentFileWriter fileWriter;
        try {
            lock.lock();
            fileWriter = fileMap.get(fullName).getUpdateFailFileWriter();
        } finally {
            lock.unlock();
        }
        fileWriter.writeToFile(oid.toString());
    }
}
