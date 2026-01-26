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

   Source File Name = SdbTaskDao.java

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
import com.sequoiadb.schedule.common.TaskEntityTranslator;
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.dao.TaskDao;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.metasource.config.SequoiadbConfig;
import com.sequoiadb.schedule.metasource.template.DataSourceWrapper;
import com.sequoiadb.schedule.metasource.template.ITransaction;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplate;
import com.sequoiadb.schedule.metasource.template.SequoiadbTransaction;
import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import javax.annotation.PostConstruct;

@Repository
public class SdbTaskDao implements TaskDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbTaskDao.class);
    private SequoiadbTemplate template;
    private static final String CL_TASK = "TASK";
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;
    private static final String INDEX_NAME = "idx_task_id";
    private static final String SCHEDULE_ID_INDEX_NAME = "idx_schedule_id";

    @Autowired
    public SdbTaskDao(DataSourceWrapper dataSourceWrapper) {
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
            template.collectionSpace().createCollection(CL_TASK);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new ScheduleServerMetaSourceException(
                        "failed to create collection:" + template.getSystemCSName() + "." + CL_TASK,
                        e);
            }
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create collection:" + template.getSystemCSName() + "." + CL_TASK, e);
        }
    }

    private void ensureIndex() throws ScheduleServerMetaSourceException {
        BasicBSONObject indexBson = new BasicBSONObject();
        indexBson.put(FieldName.Task.FIELD_ID, 1);
        try {
            template.collection(CL_TASK).ensureIndex(INDEX_NAME, indexBson, true);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to create index:csName="
                    + template.getSystemCSName() + ",clName=" + CL_TASK + ",index=" + INDEX_NAME,
                    e);
        }

        BasicBSONObject scheduleIdIndexBson = new BasicBSONObject();
        scheduleIdIndexBson.put(FieldName.Task.FIELD_SCHEDULE_ID, 1);
        try {
            template.collection(CL_TASK).ensureIndex(SCHEDULE_ID_INDEX_NAME, scheduleIdIndexBson,
                    false);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create index:csName=" + template.getSystemCSName() + ",clName="
                            + CL_TASK + ",index=" + SCHEDULE_ID_INDEX_NAME,
                    e);
        }
    }

    @Override
    public MetaCursor listTask(BSONObject condition, BSONObject orderBy, long skip, long limit)
            throws Exception {
        return template.collection(CL_TASK).find(condition, null, orderBy, skip, limit);
    }

    @Override
    public long countTask(BSONObject condition) throws Exception {
        return template.collection(CL_TASK).count(condition);
    }

    @Override
    public TaskEntity queryOne(BSONObject matcher, BSONObject orderBy) throws Exception {
        BSONObject result = template.collection(CL_TASK).findOne(matcher, null, orderBy);
        if (null == result) {
            return null;
        }

        return TaskEntityTranslator.fromBSONObject(result);
    }

    @Override
    public void insert(TaskEntity info, ITransaction t) throws Exception {
        BSONObject obj = TaskEntityTranslator.toBSONObject(info);
        template.collection(CL_TASK).insert(obj, (SequoiadbTransaction) t);
    }

    @Override
    public void updateByTaskId(String taskId, BSONObject newValue) throws Exception {
        BSONObject matcher = new BasicBSONObject(FieldName.Task.FIELD_ID, taskId);
        BSONObject modifier = new BasicBSONObject("$set", newValue);
        template.collection(CL_TASK).update(matcher, modifier);
    }

    @Override
    public void update(BSONObject matcher, BSONObject modifier) throws Exception {
        template.collection(CL_TASK).update(matcher, modifier);
    }

    @Override
    public TaskEntity queryById(String taskId) throws Exception {
        BSONObject matcher = new BasicBSONObject(FieldName.Task.FIELD_ID, taskId);
        BSONObject result = template.collection(CL_TASK).findOne(matcher, null, null);
        if (null == result) {
            return null;
        }
        return TaskEntityTranslator.fromBSONObject(result);
    }

    @Override
    public void delete(BSONObject matcher, ITransaction t) throws Exception {
        template.collection(CL_TASK).delete(matcher, (SequoiadbTransaction) t);
    }
}
