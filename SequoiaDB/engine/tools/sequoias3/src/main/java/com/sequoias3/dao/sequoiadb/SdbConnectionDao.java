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

   Source File Name = SdbConnectionDao.java

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
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SdbConnectionDao implements ConnectionDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbConnectionDao.class);

    private Sequoiadb sdb;

    public SdbConnectionDao(SdbDataSourceWrapper sdbDatasourceWrapper) throws S3ServerException{
        sdb = sdbDatasourceWrapper.getSequoiadb();
    }

    public Sequoiadb getConnection() {
        return sdb;
    }

    public void setSdb(Sequoiadb sdb) {
        this.sdb = sdb;
    }

    public void setTransTimeOut(int timeSecond) {
        try {
            BSONObject bsonObject = new BasicBSONObject();
            bsonObject.put("TransTimeout", timeSecond);
            sdb.setSessionAttr(bsonObject);
        }catch (Exception e){
            logger.error("setSessionAttr failed.", e);
        }
    }

    public int getTransTimeOut(){
        int transTimeout = 0;
        try {
            BSONObject bsonObject = sdb.getSessionAttr();
            transTimeout = (int) bsonObject.get("TransTimeout");
        }catch (Exception e){
            logger.error("setSessionAttr failed.", e);
        }
        return transTimeout;
    }
}
