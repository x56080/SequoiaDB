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

   Source File Name = SdbTransactionLockDao.java

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
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.dao.TransactionLockDao;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
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
public class SdbTransactionLockDao implements TransactionLockDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbTransactionLockDao.class);
    private static final String CL_TRANSACTION_LOCK = "TRANSACTION_LOCK";
    private static final String FIELD_LOCK_KEY = "lock_key";
    private static final String FIELD_LOCK_VALUE = "lock_value";
    private static final String LOCK_KEY_VALUE_INDEX = "lock_key_value_index";
    private final SequoiadbTemplate template;
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;
    @Autowired
    public SdbTransactionLockDao(DataSourceWrapper dataSourceWrapper)
            throws ScheduleServerMetaSourceException {
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
        BSONObject def = new BasicBSONObject(FIELD_LOCK_KEY, 1);
        def.put(FIELD_LOCK_VALUE, 1);
        this.template.collection(CL_TRANSACTION_LOCK)
                .ensureIndex(LOCK_KEY_VALUE_INDEX, def, true);
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

    private void ensureTable() throws ScheduleServerMetaSourceException {
        try {
            template.collectionSpace()
                    .createCollection(CL_TRANSACTION_LOCK);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new ScheduleServerMetaSourceException("failed to create collection:"
                        + template.getSystemCSName() + "." + CL_TRANSACTION_LOCK, e);
            }
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to create collection:"
                    + template.getSystemCSName() + "." + CL_TRANSACTION_LOCK, e);
        }
    }

    @Override
    public void update(BSONObject matcher, BSONObject update, ITransaction t) {
        template.collection(CL_TRANSACTION_LOCK).update(matcher,
                update, (SequoiadbTransaction) t);
    }

    @Override
    public void insert(BSONObject lockInfo, ITransaction t) {
        template.collection(CL_TRANSACTION_LOCK).insert(lockInfo,
                (SequoiadbTransaction) t);
    }

    @Override
    public void upsert(BSONObject lockInfo) {
        BSONObject modifier = new BasicBSONObject("$set", lockInfo);
        template.collection(CL_TRANSACTION_LOCK).upsert(lockInfo,
                modifier);
    }

    @Override
    public void delete(BSONObject matcher, ITransaction t) {
        template.collection(CL_TRANSACTION_LOCK).delete(matcher,
                (SequoiadbTransaction) t);
    }
}
