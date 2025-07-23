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

   Source File Name = CriticalModeTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.rg;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

public class CriticalModeTest {
    private static Sequoiadb sdb = null;
    private static ReplicaGroup rg = null;
    private static final String GZ = "GuangZhou";

    @BeforeClass
    public static void setUpBeforeClass() {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        rg = sdb.getReplicaGroup(Constants.GROUPNAME);
    }

    @AfterClass
    public static void tearDownAfterClass() {
        sdb.close();
    }

    @Test
    public void testCriticalModeWithNodeName() {
        BSONObject options = new BasicBSONObject();
        Node master = rg.getMaster();
        options.put("NodeName", master.getNodeName());
        options.put("MinKeepTime", 10);
        options.put("MaxKeepTime", 100);

        rg.startCriticalMode(options);
        Assert.assertTrue(isCriticalMode());
        rg.stopCriticalMode();
        Assert.assertFalse(isCriticalMode());
    }

    @Test
    public void testCriticalModeWithLocation() {
        Node master = rg.getMaster();
        master.setLocation(GZ);

        BSONObject options = new BasicBSONObject();
        options.put("Location", GZ);
        options.put("MinKeepTime", 10);
        options.put("MaxKeepTime", 100);

        rg.startCriticalMode(options);
        Assert.assertTrue(isCriticalMode());

        rg.stopCriticalMode();
        Assert.assertFalse(isCriticalMode());

        master.setLocation("");
    }

    @Test
    public void testInvalidArg() {
        BSONObject options = new BasicBSONObject();
        Node master = rg.getMaster();
        options.put("NodeName", master.getNodeName());
        // min > max
        options.put("MinKeepTime", 1000);
        options.put("MaxKeepTime", 100);
        try {
            rg.startCriticalMode(options);
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }

        // random key
        options.put("MinKeepTime", 10);
        options.put("abc", "");
        try {
            rg.startCriticalMode(options);
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }

        // option is null
        try {
            rg.startCriticalMode(null);
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }
    }

    private boolean isCriticalMode() {
        BSONObject match = new BasicBSONObject();
        match.put("GroupID", rg.getId());
        try (DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_GROUPMODES, match, null, null)) {
            while (cursor.hasNext()) {
                BSONObject o = cursor.getNext();
                String mode = (String) o.get("GroupMode");
                if (mode != null && mode.equals("critical")) {
                    return true;
                }
            }
        }
        return false;
    }
}
