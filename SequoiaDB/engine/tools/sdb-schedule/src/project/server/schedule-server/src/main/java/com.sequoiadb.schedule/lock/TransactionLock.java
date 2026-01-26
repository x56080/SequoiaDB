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

   Source File Name = TransactionLock.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.lock;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.dao.TransactionFactory;
import com.sequoiadb.schedule.dao.TransactionLockDao;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.metasource.template.ITransaction;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TransactionLock {
    private static final Logger logger = LoggerFactory.getLogger(TransactionLock.class);
    private static final String FIELD_LOCK_KEY = "lock_key";
    private static final String FIELD_LOCK_VALUE = "lock_value";
    private static final String SDB_UPDATE_SET_OPTIONS = "$set";

    private TransactionLockDao transactionLockDao;

    private ITransaction transaction;

    private String lockKey;
    private String lockValue;

    TransactionLock(TransactionLockDao transactionLockDao, TransactionFactory transactionFactory,
            String lockKey, String lockValue) {
        this.transactionLockDao = transactionLockDao;
        this.transaction = transactionFactory.createTransaction();
        this.lockKey = lockKey;
        this.lockValue = lockValue;
    }

    public void lock() throws Exception {
        AcquiredLockResult result = null;
        try {
            result = acquireLock();
        }
        catch (Exception e) {
            throw new Exception("failed to get transaction lock, lockKey:" + lockKey
                    + ", lockValue:" + lockValue, e);
        }

        if (result.getType() == ResultType.SUCCESS) {
            return;
        }
        if (result.getType() == ResultType.FAILED) {
            throw result.getException();
        }
        throw new ScheduleServerException(ScheduleServerError.TRANSACTION_LOCK_TIMEOUT,
                "failed to get transaction lock, lockKey:" + lockKey + ", lockValue:" + lockValue,
                result.getException());
    }

    private AcquiredLockResult acquireLock() {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FIELD_LOCK_KEY, lockKey);
        matcher.put(FIELD_LOCK_VALUE, lockValue);
        BSONObject modifier = new BasicBSONObject(SDB_UPDATE_SET_OPTIONS, matcher);
        try {
            transaction.begin();
            transactionLockDao.update(matcher, modifier, transaction);
            return new AcquiredLockResult(ResultType.SUCCESS, null);
        }
        catch (BaseException e) {
            transaction.rollback();
            if (isAcquiredLockTimeOutError(e.getErrorCode())) {
                return new AcquiredLockResult(ResultType.WAIT_LOCK_TIMEOUT, e);
            }
            else {
                return new AcquiredLockResult(ResultType.FAILED, e);
            }
        }
        catch (Exception e) {
            transaction.rollback();
            return new AcquiredLockResult(ResultType.FAILED, e);
        }
    }

    private boolean isAcquiredLockTimeOutError(int errorCode) {
        return errorCode == SDBError.SDB_TIMEOUT.getErrorCode();
    }

    public void unlock() {
        try {
            transaction.commit();
        }
        catch (Exception e) {
            // 事务提交失败，也会释放db连接，这里忽略异常，不影响业务
            logger.warn("unlock failed, lockPath:{}", lockKey, e);
        }
    }
}
