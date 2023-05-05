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

import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Routine {
    private static final Logger logger = LoggerFactory.getLogger(Routine.class);
    private Param param;
    private SequoiadbDatasource datasource;
    private JobMgr jobMgr;
    private FileMgr fileMgr;

    public Routine(Param param, SequoiadbDatasource datasource, JobMgr jobMgr, FileMgr fileMgr) {
        this.param = param;
        this.datasource = datasource;
        this.jobMgr = jobMgr;
        this.fileMgr = fileMgr;
    }

    public void run() {
        int threadNum = param.getThreadNum();
        Thread[] threads = new Thread[threadNum];

        // 加载任务
        jobMgr.loadJobRecords();

        // 启动 works 工作
        for (int i = 0; i < threadNum; i++) {
            threads[i] = new Thread(new Worker(param, datasource, jobMgr, fileMgr), "Thread" + i);
        }
        for (int i = 0; i < threadNum; i++) {
            threads[i].start();
        }
        for (int i = 0; i < threadNum; i++) {
            try {
                threads[i].join();
            } catch (Exception e) {
                String errMsg = "Main thread join error";
                logger.error(errMsg + ", " + e.getMessage());
                throw new BaseException(SDBError.SDB_SYS, errMsg, e);
            }
        }
    }
}
