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

   Source File Name = SdbTaskProgressDao.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.dao.sdb;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.dao.TaskProgressDao;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.metasource.template.DataSourceWrapper;
import com.sequoiadb.schedule.metasource.template.ITransaction;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplate;
import com.sequoiadb.schedule.metasource.template.SequoiadbTransaction;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import javax.annotation.PostConstruct;

@Repository
public class SdbTaskProgressDao implements TaskProgressDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbTaskProgressDao.class);
    private SequoiadbTemplate template;
    private static final String CL_TASK_PROGRESS = "TASK_PROGRESS";
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;
    private static final String INDEX_NAME = "idx_task_id";

    @Autowired
    public SdbTaskProgressDao(DataSourceWrapper dataSourceWrapper) {
        this.template = new SequoiadbTemplate(dataSourceWrapper);
    }

    @PostConstruct
    public void init() {
        try {
            ensureTableAndIndex();
        }
        catch (Exception e) {
            logger.error("failed to create table or index", e);
            asyncRetryEnsureTableAndIndex();
        }
    }

    private void asyncRetryEnsureTableAndIndex() {
        if (retryTimer == null) {
            retryTimer = TimerFactory.createTimer("retryEnsureTableAndIndexTimer");
        }
        retryTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    logger.info("retry to create table and index");
                    ensureTableAndIndex();
                    cancel();
                }
                catch (Exception e) {
                    logger.error("failed to create table or index", e);
                }
            }
        }, RETRY_INTERVAL, RETRY_INTERVAL);
    }

    private void ensureTableAndIndex() throws ScheduleServerMetaSourceException {
        ensureTable();
        ensureIndex();
    }

    private void ensureTable() throws ScheduleServerMetaSourceException {
        try {
            template.collectionSpace()
                    .createCollection(CL_TASK_PROGRESS);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new ScheduleServerMetaSourceException("failed to create collection:"
                        + template.getSystemCSName() + "." + CL_TASK_PROGRESS, e);
            }
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to create collection:"
                    + template.getSystemCSName() + "." + CL_TASK_PROGRESS, e);
        }
    }

    private void ensureIndex() throws ScheduleServerMetaSourceException {
        BasicBSONObject indexBson = new BasicBSONObject();
        indexBson.put(FieldName.TaskProgress.FIELD_TASK_ID, 1);
        try {
            template.collection(CL_TASK_PROGRESS)
                    .ensureIndex(INDEX_NAME, indexBson, false);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create index:csName=" + template.getSystemCSName() + ",clName="
                            + CL_TASK_PROGRESS + ",index=" + INDEX_NAME,
                    e);
        }
    }

    @Override
    public MetaCursor listTaskProgress(String taskId) throws Exception {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.TaskProgress.FIELD_TASK_ID, taskId);
        return template.collection(CL_TASK_PROGRESS).find(matcher,
                null, null, 0, -1);
    }

    @Override
    public void updateTaskProgress(String taskId, String clName, long successRecordNum, long failedRecordNum, long transferRecordSize, long successLobNum, long failedLobNum, long transferLobSize) throws Exception {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.TaskProgress.FIELD_TASK_ID, taskId);
        matcher.put(FieldName.TaskProgress.FIELD_COLLECTION_NAME, clName);
        BSONObject newValue = new BasicBSONObject();
        newValue.put(FieldName.TaskProgress.FIELD_TASK_ID, taskId);
        newValue.put(FieldName.TaskProgress.FIELD_COLLECTION_NAME, clName);
        newValue.put(FieldName.TaskProgress.FIELD_SUCCESS_RECORD_NUM, successRecordNum);
        newValue.put(FieldName.TaskProgress.FIELD_FAILED_RECORD_NUM, failedRecordNum);
        newValue.put(FieldName.TaskProgress.FIELD_TRANSFER_RECORD_SIZE, transferRecordSize);
        newValue.put(FieldName.TaskProgress.FIELD_SUCCESS_LOB_NUM, successLobNum);
        newValue.put(FieldName.TaskProgress.FIELD_FAILED_LOB_NUM, failedLobNum);
        newValue.put(FieldName.TaskProgress.FIELD_TRANSFER_LOB_SIZE, transferLobSize);
        BSONObject modifier = new BasicBSONObject("$set", newValue);
        template.collection(CL_TASK_PROGRESS)
                .upsert(matcher, modifier);
    }

    @Override
    public void saveTaskProgress(BSONObject taskProgress) {
        template.collection(CL_TASK_PROGRESS)
                .insert(taskProgress);
    }

    @Override
    public void delete(BSONObject matcher, ITransaction t) throws Exception {
        template.collection(CL_TASK_PROGRESS)
                .delete(matcher, (SequoiadbTransaction) t);
    }
}
