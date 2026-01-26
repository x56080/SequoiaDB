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

   Source File Name = SdbScheduleDao.java

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
import com.sequoiadb.schedule.common.ScheduleEntityTranslator;
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.dao.ScheduleDao;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.metasource.SdbMetaCursor;
import com.sequoiadb.schedule.metasource.template.DataSourceWrapper;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplate;
import com.sequoiadb.schedule.model.ScheduleFullEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import javax.annotation.PostConstruct;

@Repository
public class SdbScheduleDao implements ScheduleDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbScheduleDao.class);
    private SequoiadbTemplate template;
    private static final String CL_SCHEDULE = "SCHEDULE";
    private static final String ID_INDEX_NAME = "idx_schedule_id";
    private static final String NAME_INDEX_NAME = "idx_schedule_name";
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;

    @Autowired
    public SdbScheduleDao(DataSourceWrapper dataSourceWrapper) {
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

    private void ensureTableAndIndex() throws ScheduleServerMetaSourceException {
        ensureTable();
        ensureIndex();
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

    private void ensureIndex() throws ScheduleServerMetaSourceException {
        BasicBSONObject idIndexBson = new BasicBSONObject();
        idIndexBson.put(FieldName.Schedule.FIELD_ID, 1);
        try {
            template.collection(CL_SCHEDULE)
                    .ensureIndex(ID_INDEX_NAME, idIndexBson, true);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create index:csName=" + template.getSystemCSName() + ",clName="
                            + CL_SCHEDULE + ",index=" + ID_INDEX_NAME,
                    e);
        }

        BasicBSONObject nameIndexBson = new BasicBSONObject();
        nameIndexBson.put(FieldName.Schedule.FIELD_NAME, 1);
        try {
            template.collection(CL_SCHEDULE)
                    .ensureIndex(NAME_INDEX_NAME, nameIndexBson, false);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create index:csName=" + template.getSystemCSName() + ",clName="
                            + CL_SCHEDULE + ",index=" + NAME_INDEX_NAME,
                    e);
        }
    }

    private void ensureTable() throws ScheduleServerMetaSourceException {
        try {
            template.collectionSpace().createCollection(CL_SCHEDULE);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new ScheduleServerMetaSourceException("failed to create collection:"
                        + template.getSystemCSName() + "." + CL_SCHEDULE, e);
            }
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to create collection:"
                    + template.getSystemCSName() + "." + CL_SCHEDULE, e);
        }
    }

    @Override
    public SdbMetaCursor query(BSONObject matcher) throws Exception {
        MetaCursor metaCursor = template.collection(CL_SCHEDULE)
                .find(matcher);
        return (SdbMetaCursor) metaCursor;
    }

    @Override
    public void updateByScheduleId(String scheduleId, BSONObject newValue) throws Exception {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.Schedule.FIELD_ID, scheduleId);
        BSONObject updator = new BasicBSONObject();
        updator.put("$set", newValue);
        template.collection(CL_SCHEDULE).update(matcher, updator);
    }

    @Override
    public void insert(ScheduleFullEntity info) throws Exception {
        BSONObject bsonObject = ScheduleEntityTranslator.FullInfo.toBSONObject(info);
        template.collection(CL_SCHEDULE).insert(bsonObject);
    }

    @Override
    public void updateByScheduleId(String scheduleId, ScheduleFullEntity info) throws Exception {
        BSONObject bsonObject = ScheduleEntityTranslator.FullInfo.toBSONObject(info);
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.Schedule.FIELD_ID, scheduleId);
        BSONObject updator = new BasicBSONObject();
        updator.put("$set", bsonObject);
        template.collection(CL_SCHEDULE).update(matcher, updator);
    }

    @Override
    public ScheduleFullEntity queryOne(String scheduleId) throws Exception {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.Schedule.FIELD_ID, scheduleId);
        BSONObject object = template.collection(CL_SCHEDULE)
                .findOne(matcher);
        if (object != null) {
            return ScheduleEntityTranslator.FullInfo.fromBSONObject(object);
        }
        return null;
    }

    @Override
    public void delete(String scheduleId) throws Exception {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.Schedule.FIELD_ID, scheduleId);
        template.collection(CL_SCHEDULE).delete(matcher);
    }

    @Override
    public long countSchedule(BSONObject condition) throws Exception {
        return template.collection(CL_SCHEDULE).count(condition);
    }

    @Override
    public MetaCursor listSchedule(BSONObject condition, BSONObject orderBy, long skip, long limit)
            throws Exception {
        return template.collection(CL_SCHEDULE).find(condition,
                null, orderBy, skip, limit);
    }
}
