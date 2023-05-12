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

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.types.BSONTimestamp;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;

public class Common {
    public static final long timestamp = System.currentTimeMillis();
    public static final String EXCLUDE_LINE_PREFIX = "#";
    public static final String IDX_DEFAULT_ID = "$id";
    public static final String IDX_USER_DEF_OID = "idx_oid"; // 更新表使用 "Oid" 字段创建的非唯一索引

    public static final int CACHE_SIZE_512KB = 512 * 1024;
    public static final int CACHE_SIZE_1MB = 1 * 1024 * 1024;
    public static final int CACHE_SIZE_2MB = 2 * 1024 * 1024;

    private static final Logger logger = LoggerFactory.getLogger(Common.class);
    private SequoiadbDatasource datasource;

    public Common(SequoiadbDatasource datasource) {
        this.datasource = datasource;
    }

    public Sequoiadb getSdb() {
        if (datasource == null) {
            return null;
        }
        Sequoiadb sdb;
        try {
            sdb = datasource.getConnection();
        } catch (InterruptedException e) {
            String errMsg = "Failed to get connection";
            logger.error(errMsg + ", " + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        }
        return sdb;
    }

    public void releaseSdb(Sequoiadb sdb) {
        if (datasource == null || sdb == null) {
            return;
        }
        datasource.releaseConnection(sdb);
    }

    public static Sequoiadb createSdb(String address, String userName, String password) {
        return new Sequoiadb(address, userName, password);
    }

    public static void closeSdb(Sequoiadb sdb) {
        if (sdb == null) {
            return;
        }
        sdb.close();
    }

    /**
     * @param sdb
     * @param collectionName cl full name
     * @return
     */
    public static DBCollection getCL(Sequoiadb sdb, String collectionName) {
        String[] array = collectionName.split("\\.");
        return sdb.getCollectionSpace(array[0]).getCollection(array[1]);
    }

    public static void checkOutputDir(String outputDir) {
        File dir = new File(outputDir);
        if (!dir.exists()) {
            String errMsg = "Output directory does not exist: " + outputDir;
            logger.error(errMsg);
            throw new BaseException(SDBError.SDB_SYS, errMsg);
        }
    }

    public static String createOutputFile(String outputDir, String fileName) {
        return createOutputFile(outputDir, fileName, true);
    }

    public static String createOutputFile(String outputDir, String fileName, boolean dropIfExist) {
        // check output directory
        checkOutputDir(outputDir);

        // build the file path
        Path dirPath = Paths.get(outputDir);
        Path filePath = Paths.get(fileName);
        // path is outputDir + fileName, can use both in linux and windows
        Path path = dirPath.resolve(filePath);

        // check and create a new file
        File file = path.toFile();
        if (file.exists()) {
            if (dropIfExist) {
                if (!file.delete()) {
                    String errMsg = "File[" + file.toString() + "] has existed, failed to drop";
                    logger.error(errMsg);
                    throw new BaseException(SDBError.SDB_FE, errMsg);
                }
            } else {
                String errMsg = "File[" + file.toString() + "] has existed, please check and delete it first";
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_FE, errMsg);
            }
        }
        try {
            file.createNewFile();
        } catch (Exception e) {
            String errMsg = "Failed to create file: " + file.toString();
            logger.error(errMsg + ", " + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        }
        logger.info("Success to create file: " + file.toString());

        return file.toString();
    }

    public static String getFileName(String filePath) {
        File file = new File(filePath);
        return file.getName();
    }

    /**
     * @param collectionName cl full name
     * @return
     */
    public static boolean isValidCollectionName(String collectionName) {
        if (collectionName == null || collectionName.isEmpty()) {
            return false;
        }
        String[] array = collectionName.split("\\.");
        if (array == null || array.length != 2 || array[0].isEmpty() || array[1].isEmpty()) {
            return false;
        }
        return true;
    }

    public static boolean isAddressUsable(String address, String userName, String password) {
        try {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb(address, userName, password);
            } finally {
                if (sdb != null) {
                    sdb.close();
                }
            }
        } catch (Exception e) {
            return false;
        }
        return true;
    }

    public static String[] getNames(String fullName) {
        String[] array = fullName.split("\\.");
        return array;
    }

    public static long timestampToLong(BSONTimestamp ts) {
        long timeMillSec = (ts.getTime() * 1000L) + (ts.getInc() / 1000L);
        return timeMillSec;
    }

    public static long printStatistics(String taskInfo, long startTM, long lastTM, int fixedCount) {
        return printStatistics(taskInfo, startTM, lastTM, fixedCount, 300); // 大于 5min 打印一次统计信息
    }

    public static long printStatistics(String taskInfo, long startTM, long lastTM, int fixedCount, int intervalSec) {
        long currentTM = System.currentTimeMillis();
        if ((currentTM - lastTM) >= (intervalSec * 1000L)) {
            long deltaTM = currentTM - startTM;
            long sec = deltaTM / 1000L;
            long millSec = deltaTM % 1000L;
            logger.info("Task[" + taskInfo + "] worker[" + Thread.currentThread().getName() +
                    "] " + "has run: " + sec + "." + millSec + "(secs), and has handled: " + fixedCount + " records");
            return currentTM;
        } else {
            return lastTM;
        }
    }

}
