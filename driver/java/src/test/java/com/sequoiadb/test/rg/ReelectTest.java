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

   Source File Name = ReelectTest.java

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
import com.sequoiadb.base.Node.NodeStatus;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class ReelectTest {

    private static Sequoiadb sdb;
    private static ReplicaGroup rg = null;
    //	private static Node node = null;
    private static DBCursor cursor;
    private static final int PORT = 54300;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        // shard
        rg = sdb.getReplicaGroup(Constants.GROUPNAME);
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void reelectTest() {
        Node master1 = rg.getMaster();
        rg.reelect();
        Node master2 = rg.getMaster();
        if(master1.getNodeId() == master2.getNodeId()){
            System.out.println("reelect failure");
        }else {
            System.out.println("reelect success");
        }
    }

    @Test
    public void reelectUseOption() {
        BSONObject options = new BasicBSONObject();
        options.put("Seconds",10);
        Node slave = rg.getSlave();
        options.put("NodeID",slave.getNodeId());

        rg.reelect(options);

        if (slave.getNodeId() == rg.getMaster().getNodeId()){
            System.out.println("reelect success");
        }else {
            System.out.println("reelect failure");
        }
    }


}
