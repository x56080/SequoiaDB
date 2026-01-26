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

   Source File Name = SdbMetaCursor.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

package com.sequoiadb.schedule.metasource;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SdbMetaCursor implements MetaCursor {
    private static final Logger logger = LoggerFactory.getLogger(SdbMetaCursor.class);
    private CloseCallback closeCallback;
    private Sequoiadb sdb;
    private DBCursor cursor;

    public SdbMetaCursor(CloseCallback closeCallback, Sequoiadb sdb, DBCursor cursor) {
        this.closeCallback = closeCallback;
        this.sdb = sdb;
        this.cursor = cursor;
    }

    @Override
    public boolean hasNext() throws BaseException {
        if (null == cursor) {
            return false;
        }
        return cursor.hasNext();
    }

    @Override
    public BSONObject getNext() throws BaseException {
        if (null == cursor) {
            return null;
        }

        BSONObject o = cursor.getNext();
        if (null != o) {
            o.removeField("_id");
        }
        return o;

    }

    @Override
    public void close() {
        try {
            if (null != cursor) {
                cursor.close();
            }
        }
        catch (Exception e) {
            logger.warn("close cursor failed", e);
        }

        if (null != sdb && null != closeCallback) {
            closeCallback.close(sdb);
        }

        cursor = null;
        sdb = null;
    }

    public interface CloseCallback {
        public void close(Sequoiadb sdb);
    }
}
