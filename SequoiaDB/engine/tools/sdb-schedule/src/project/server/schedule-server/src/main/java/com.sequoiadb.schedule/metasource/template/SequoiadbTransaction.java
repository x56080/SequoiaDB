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

   Source File Name = SequoiadbTransaction.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

package com.sequoiadb.schedule.metasource.template;

import com.sequoiadb.base.Sequoiadb;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SequoiadbTransaction implements ITransaction {
    private static final Logger logger = LoggerFactory.getLogger(SequoiadbTransaction.class);

    private DataSourceWrapper datasourceWrapper;
    private Sequoiadb sdb = null;

    public SequoiadbTransaction(DataSourceWrapper datasourceWrapper) {
        this.datasourceWrapper = datasourceWrapper;
    }

    @Override
    public void begin() throws Exception {
        if (null == sdb) {
            try {
                sdb = datasourceWrapper.getConnection();
                sdb.beginTransaction();
            }
            catch (Exception e) {
                releaseConnection();
                throw e;
            }
        }
    }

    @Override
    public void commit() {
        if (null != sdb) {
            try {
                sdb.commit();
            }
            finally {
                releaseConnection();
            }
        }
    }

    private void releaseConnection() {
        try {
            datasourceWrapper.releaseConnection(sdb);
        }
        catch (Exception e) {
            logger.warn("release connection failed:sdb={}", sdb, e);
        }

        sdb = null;
    }

    @Override
    public void rollback() {
        if (null != sdb) {
            try {
                sdb.rollback();
            }
            finally {
                releaseConnection();
            }
        }
    }

    public Sequoiadb getSequoiadb() {
        return sdb;
    }
}
