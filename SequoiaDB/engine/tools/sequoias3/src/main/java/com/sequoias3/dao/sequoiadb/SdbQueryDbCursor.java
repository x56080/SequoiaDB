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

   Source File Name = SdbQueryDbCursor.java

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

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.dao.QueryDbCursor;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SdbQueryDbCursor implements QueryDbCursor {
    private static final Logger logger = LoggerFactory.getLogger(SdbQueryDbCursor.class);

    private Sequoiadb sdb;
    private DBCursor cursor;

    public SdbQueryDbCursor(Sequoiadb sdb, DBCursor cursor){
        this.sdb = sdb;
        this.cursor = cursor;
    }

    public Sequoiadb getSdb() {
        return sdb;
    }

    public DBCursor getCursor() {
        return cursor;
    }

    public void setSdb(Sequoiadb sdb) {
        this.sdb = sdb;
    }

    public void setCursor(DBCursor cursor) {
        this.cursor = cursor;
    }

    @Override
    public boolean hasNext() throws S3ServerException {
        if (null == cursor) {
            return false;
        }

        try {
            return cursor.hasNext();
        }
        catch (BaseException e) {
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "failed to hasNext", e);
        }
        catch (Exception e) {
            throw new S3ServerException(S3Error.DAO_DB_ERROR,
                    "failed to hasNext", e);
        }
    }

    @Override
    public BSONObject getNext() throws S3ServerException {
        if (null == cursor) {
            return null;
        }

        try {
            return cursor.getNext();
        }catch (BaseException e) {
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "failed to getNext", e);
        } catch (Exception e) {
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "failed to getNext", e);
        }
    }

    @Override
    public BSONObject getCurrent() throws S3ServerException {
        if (null == cursor) {
            return null;
        }

        try {
            return cursor.getCurrent();
        }catch (BaseException e) {
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "failed to getCurrent", e);
        } catch (Exception e) {
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "failed to getCurrent", e);
        }
    }


}
