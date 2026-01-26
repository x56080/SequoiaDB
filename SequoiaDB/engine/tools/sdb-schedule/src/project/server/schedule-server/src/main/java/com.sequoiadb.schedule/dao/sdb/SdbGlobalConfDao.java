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

   Source File Name = SdbGlobalConfDao.java

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
import com.sequoiadb.schedule.dao.GlobalConfDao;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.metasource.template.DataSourceWrapper;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplate;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import javax.annotation.PostConstruct;

@Repository
public class SdbGlobalConfDao implements GlobalConfDao {

    private static final Logger logger = LoggerFactory.getLogger(SdbSiteDao.class);
    private SequoiadbTemplate template;
    private static final String CL_GLOBAL_CONF = "GLOBAL_CONF";

    private static final String INDEX_NAME = "idx_conf_key";

    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;

    @Autowired
    public SdbGlobalConfDao(DataSourceWrapper dataSourceWrapper) {
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
            template.collectionSpace().createCollection(CL_GLOBAL_CONF);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new ScheduleServerMetaSourceException("failed to create collection:"
                        + template.getSystemCSName() + "." + CL_GLOBAL_CONF, e);
            }
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to create collection:"
                    + template.getSystemCSName() + "." + CL_GLOBAL_CONF, e);
        }
    }

    private void ensureIndex() throws ScheduleServerMetaSourceException {
        BasicBSONObject indexBson = new BasicBSONObject();
        indexBson.put(FieldName.GlobalConf.FIELD_GLOBAL_CONF_KEY, 1);
        try {
            template.collection(CL_GLOBAL_CONF).ensureIndex(INDEX_NAME,
                    indexBson, true);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create index:csName=" + template.getSystemCSName() + ",clName="
                            + CL_GLOBAL_CONF + ",index=" + INDEX_NAME,
                    e);
        }
    }

    @Override
    public MetaCursor listGlobalConf(BSONObject condition, BSONObject orderBy, long skip, long limit) throws Exception {
        return template.collection(CL_GLOBAL_CONF)
                .find(condition, null,
                orderBy, skip, limit);
    }

    @Override
    public long countGlobalConf(BSONObject condition) throws Exception {
        return template.collection(CL_GLOBAL_CONF).count(condition);
    }

    @Override
    public void updateGlobalConf(String key, String value) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.GlobalConf.FIELD_GLOBAL_CONF_KEY, key);
        BSONObject modifier = new BasicBSONObject("$set", new BasicBSONObject(FieldName.GlobalConf.FIELD_GLOBAL_CONF_VALUE, value));
        template.collection(CL_GLOBAL_CONF)
                .update(matcher, modifier);
    }

    @Override
    public String getGlobalConfValue(String key) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.GlobalConf.FIELD_GLOBAL_CONF_KEY, key);
        BSONObject obj = template.collection(CL_GLOBAL_CONF)
                .findOne(matcher);
        if (obj != null) {
            return (String) obj.get(FieldName.GlobalConf.FIELD_GLOBAL_CONF_VALUE);
        }
        return null;
    }

    @Override
    public void insert(String key, String value, String desc) {
        BSONObject obj = new BasicBSONObject();
        obj.put(FieldName.GlobalConf.FIELD_GLOBAL_CONF_KEY, key);
        obj.put(FieldName.GlobalConf.FIELD_GLOBAL_CONF_VALUE, value);
        obj.put(FieldName.GlobalConf.FIELD_GLOBAL_CONF_DESC, desc);
        template.collection(CL_GLOBAL_CONF)
                .insert(obj);
    }
}
