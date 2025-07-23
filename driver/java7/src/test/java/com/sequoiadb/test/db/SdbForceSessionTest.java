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

   Source File Name = SdbForceSessionTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.db;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

public class SdbForceSessionTest {
    private static Sequoiadb sdb;
    private static Sequoiadb forceSdb;

    @Before
    public void setUp() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        forceSdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @After
    public void tearDown() throws Exception {
        sdb.close();
        forceSdb.close();
    }

    public SessionInfo getSessionInfo(Sequoiadb db){
        BSONObject matcher = new BasicBSONObject();
        matcher.put("Global", false);

        try (DBCursor cursor = db.getSnapshot(Sequoiadb.SDB_SNAP_SESSIONS_CURRENT, matcher, null, null)) {
            BSONObject obj = cursor.getNext();
            Assert.assertNotNull(obj);

            long sessionId = (Long)obj.get("SessionID");
            String nodeName = (String)obj.get("NodeName");
            return new SessionInfo(sessionId, nodeName);
        }
    }

    @Test
    public void forceSessionTest() {
        // case 1: error session id
        try {
            sdb.forceSession(-1);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_PMD_SESSION_NOT_EXIST.getErrorCode(), e.getErrorCode());
        }

        // case 2: normal session id
        SessionInfo info = getSessionInfo(forceSdb);
        sdb.forceSession(info.getSessionId());
        // check
        try {
            forceSdb.getSessionAttr(false);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_NETWORK.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void forceSessionWithOptions() {
        SessionInfo info = getSessionInfo(forceSdb);
        BSONObject options = new BasicBSONObject();

        options.put("NodeName", info.getNodeName());
        sdb.forceSession(info.getSessionId(), options);
        // check
        try {
            forceSdb.getSessionAttr(false);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_NETWORK.getErrorCode(), e.getErrorCode());
        }
    }

    static class SessionInfo {
        private final long sessionId;
        private final String nodeName;

        public SessionInfo(long sessionId, String nodeName) {
            this.sessionId = sessionId;
            this.nodeName = nodeName;
        }

        public long getSessionId() {
            return sessionId;
        }

        public String getNodeName() {
            return nodeName;
        }
    }
}
