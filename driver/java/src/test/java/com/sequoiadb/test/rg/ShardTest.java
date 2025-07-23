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

   Source File Name = ShardTest.java

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
import org.junit.*;

import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class ShardTest {

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

    @Ignore
    @Test
    public void traverseClassShard() {
        // isCatalog
        boolean cata = rg.isCatalog();
        assertFalse(cata);
        // getSequoiadb
        Sequoiadb s = rg.getSequoiadb();
        assertTrue(s.equals(sdb));
        // getId
        int id = 0;
        id = rg.getId();
        assertTrue(id != 0);
        // getShardName
        String name = "";
        name = rg.getGroupName();
        assertTrue(name.equals(Constants.GROUPNAME));
        // getNodeNum
        int num = 0;
        num = rg.getNodeNum(NodeStatus.SDB_NODE_ALL);
        assertTrue(num != 0);
        // getDetail
        BSONObject detail = null;
        detail = rg.getDetail();
        assertTrue(detail != null);
        // getMaster
        Node master = null;
        master = rg.getMaster();
        assertTrue(master != null);
        // getSlave
        Node slave = null;
        slave = rg.getSlave();
        assertTrue(slave != null);
        // getNode
        Node node1 = null;
        node1 = rg.getNode(master.getNodeName());
        assertTrue(node1 != null);
        Node node2 = null;
        node2 = rg.getNode(slave.getHostName(), slave.getPort());
        assertTrue(node2 != null);
        // createNode
        Map<String, String> conf = new HashMap<String, String>();
        conf.put("logfilesz", "32");
        Node node = rg.createNode(Constants.HOST, PORT, Constants.DATAPATH4, conf);
        assertTrue(node != null);
        // start
        node.start();
        try {
            Thread.currentThread().sleep(15000);
        } catch (InterruptedException e) {
        }
        Sequoiadb ddb = null;
        ddb = new Sequoiadb(Constants.HOST, PORT, "", "");
        assertTrue(ddb != null);
        // stop
        node.stop();
        try {
            Thread.currentThread().sleep(3000);
        } catch (InterruptedException e) {
        }
        ddb = null;
        try {
            ddb = new Sequoiadb(Constants.HOST, 54300, "", "");
        } catch (BaseException e) {
            assertTrue(e.getErrorType().equals("SDB_NETWORK"));
        }
        assertTrue(ddb == null);
        // removeNode
        rg.removeNode(Constants.HOST, PORT, null);
        node = null;
        node = rg.getNode(Constants.HOST, PORT);
        assertTrue(node == null);
    }

}
