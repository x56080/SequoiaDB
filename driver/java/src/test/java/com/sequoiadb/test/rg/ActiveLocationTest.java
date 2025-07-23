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

   Source File Name = ActiveLocationTest.java

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

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

public class ActiveLocationTest {
    private static Sequoiadb sdb = null;
    private static ReplicaGroup rg = null;
    private static final String GZ = "GuangZhou";
    private static final String ACTIVE_LOCATION = "ActiveLocation";


    @BeforeClass
    public static void setUpBeforeClass() {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        rg = sdb.getReplicaGroup(Constants.GROUPNAME);
    }

    @AfterClass
    public static void tearDownAfterClass() {
        sdb.close();
    }

    @Before
    public void setUp() {
        Node master = rg.getMaster();
        master.setLocation(GZ);
    }

    @After
    public void dropDown() {
        Node master = rg.getMaster();
        master.setLocation("");
        rg.setActiveLocation("");
    }

    @Test
    public void testActiveLocation() {
        // test active location
        String activeInfo = "";
        rg.setActiveLocation(GZ);
        activeInfo = getActiveInfo();
        Assert.assertEquals(activeInfo, GZ);

        // test clean active location
        rg.setActiveLocation("");
        activeInfo = getActiveInfo();
        Assert.assertEquals(activeInfo, "");

        // test active location with null
        try {
            rg.setActiveLocation(null);
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }
    }

    private String getActiveInfo() {
        BSONObject match = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        match.put("GroupID", rg.getId());
        selector.put(ACTIVE_LOCATION, "");
        try (DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_GROUPS, match, selector, null)) {
            while (cursor.hasNext()) {
                return (String) cursor.getNext().get(ACTIVE_LOCATION);
            }
        }
        return null;
    }
}
