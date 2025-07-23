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

   Source File Name = SdbTransaction.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.Transaction;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

@Repository
public class SdbTransaction implements Transaction {
    private static final Logger logger = LoggerFactory.getLogger(SdbTransaction.class);

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Override
    public void begin(ConnectionDao connection) throws S3ServerException {
        Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
        try {
            sdb.beginTransaction();
        }catch (Exception e){
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            throw new S3ServerException(S3Error.DAO_TRANSACTION_BEGIN_ERROR, "startTransaction failed", e);
        }
    }

    @Override
    public void commit(ConnectionDao connection) throws S3ServerException {
        SdbConnectionDao sdbConnection = (SdbConnectionDao)connection;
        Sequoiadb sdb = sdbConnection.getConnection();
        if (sdb == null){
            logger.error("commit sdb is null.");
            return;
        }
        try {
            sdb.commit();
        }catch (Exception e){
            throw new S3ServerException(S3Error.DAO_TRANSACTION_COMMIT_FAILED, "commit transaction failed", e);
        }
    }

    @Override
    public void rollback(ConnectionDao connection) {
        SdbConnectionDao sdbConnection = (SdbConnectionDao)connection;
        Sequoiadb sdb = sdbConnection.getConnection();
        if (sdb == null){
            return;
        }
        try {
            sdb.rollback();
        }catch (Exception e){
            logger.error("rollback failed.", e);
        }
    }
}
