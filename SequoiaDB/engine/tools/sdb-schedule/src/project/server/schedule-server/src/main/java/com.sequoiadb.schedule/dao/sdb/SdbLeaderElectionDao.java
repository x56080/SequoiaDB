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

   Source File Name = SdbLeaderElectionDao.java

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
import com.sequoiadb.schedule.config.ElectionConfig;
import com.sequoiadb.schedule.dao.LeaderElectionDao;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
import com.sequoiadb.schedule.metasource.config.SequoiadbConfig;
import com.sequoiadb.schedule.metasource.template.DataSourceWrapper;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplate;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplateWithTimeout;
import com.sequoiadb.schedule.model.ScheduleServerLeaderInfo;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;

@Repository
public class SdbLeaderElectionDao implements LeaderElectionDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbLeaderElectionDao.class);
    private SequoiadbTemplate template;
    @Autowired
    private SequoiadbConfig sequoiadbConfig;
    private static final String CL_SCHEDULE_SERVER_ELECTION = "SCHEDULE_SERVER_ELECTION";
    private static final String INDEX_NAME = "idx_service_type";
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;
    private DataSourceWrapper dataSourceWrapper;

    public SdbLeaderElectionDao(ElectionConfig electionConfig,
            DataSourceWrapper dataSourceWrapper) {
        this.dataSourceWrapper = dataSourceWrapper;
        // 为避免由于网络问题导致续约等 db 操作长时间阻塞，影响选举，这里需要设置一个较短的超时时间
        this.template = new SequoiadbTemplateWithTimeout(dataSourceWrapper,
                electionConfig.getRenewInterval() * 1000);
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

    @PreDestroy
    public void destroy() {
        if (retryTimer != null) {
            retryTimer.cancel();
        }
    }

    @Override
    public ScheduleServerLeaderInfo queryOne() throws ScheduleServerMetaSourceException {
        try {
            BSONObject res = template
                    .collection(CL_SCHEDULE_SERVER_ELECTION)
                    .findOne(null);
            if (res == null) {
                return null;
            }
            return new ScheduleServerLeaderInfo(res);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to query leader info", e);
        }
    }

    @Override
    public void insert(ScheduleServerLeaderInfo leaderInfo)
            throws ScheduleServerMetaSourceException {
        try {
            template.collection(CL_SCHEDULE_SERVER_ELECTION)
                    .insert(leaderInfo.toBson());
        }
        catch (Exception e) {
            ScheduleServerError error = ScheduleServerError.META_SOURCE_ERROR;
            if (e instanceof BaseException) {
                BaseException baseException = (BaseException) e;
                if (baseException.getErrorCode() == SDBError.SDB_IXM_DUP_KEY.getErrorCode()) {
                    error = ScheduleServerError.DUPLICATE_RECORD;
                }
            }
            throw new ScheduleServerMetaSourceException(error, "failed to insert leader info", e);
        }
    }

    @Override
    public ScheduleServerLeaderInfo renew(ScheduleServerLeaderInfo oldLeaderInfo)
            throws ScheduleServerMetaSourceException {
        try {
            BSONObject matcher = oldLeaderInfo.toBson();
            BSONObject modifier = new BasicBSONObject("$inc",
                    new BasicBSONObject(FieldName.ScheduleServerElection.LEASE_NUM, 1));
            modifier.put("$set", new BasicBSONObject(FieldName.ScheduleServerElection.UPDATE_TIME,
                    System.currentTimeMillis()));

            BSONObject res = template
                    .collection(CL_SCHEDULE_SERVER_ELECTION)
                    .queryAndUpdate(matcher, null, modifier, true);
            if (res == null) {
                return null;
            }
            return new ScheduleServerLeaderInfo(res);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to renew leader ", e);
        }
    }

    @Override
    public ScheduleServerLeaderInfo updateAndReturnNew(ScheduleServerLeaderInfo newLeaderInfo,
            ScheduleServerLeaderInfo oldLeaderInfo) throws ScheduleServerMetaSourceException {
        try {
            BSONObject matcher = oldLeaderInfo.toBson();
            BSONObject modifier = new BasicBSONObject("$set", newLeaderInfo.toBson());

            BSONObject res = template
                    .collection(CL_SCHEDULE_SERVER_ELECTION)
                    .queryAndUpdate(matcher, null, modifier, true);
            if (res == null) {
                return null;
            }
            return new ScheduleServerLeaderInfo(res);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to update leader info", e);
        }
    }

    @Override
    public void useTempConnection() {
        this.template = new TempSequoiadbTemplate(dataSourceWrapper, sequoiadbConfig);
    }

    private void ensureTableAndIndex() throws ScheduleServerMetaSourceException {
        ensureTable();
        ensureIndex();
    }

    private void ensureTable() throws ScheduleServerMetaSourceException {
        try {
            template.collectionSpace()
                    .createCollection(CL_SCHEDULE_SERVER_ELECTION);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new ScheduleServerMetaSourceException("failed to create collection:"
                        + template.getSystemCSName() + "." + CL_SCHEDULE_SERVER_ELECTION, e);
            }
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to create collection:"
                    + template.getSystemCSName() + "." + CL_SCHEDULE_SERVER_ELECTION, e);
        }
    }

    private void ensureIndex() throws ScheduleServerMetaSourceException {
        BasicBSONObject indexBson = new BasicBSONObject();
        indexBson.put(FieldName.ScheduleServerElection.SERVER_TYPE, 1);
        try {
            template.collection(CL_SCHEDULE_SERVER_ELECTION)
                    .ensureIndex(INDEX_NAME, indexBson, true);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create index:csName=" + template.getSystemCSName() + ",clName="
                            + CL_SCHEDULE_SERVER_ELECTION + ",index=" + INDEX_NAME,
                    e);
        }
    }
}
