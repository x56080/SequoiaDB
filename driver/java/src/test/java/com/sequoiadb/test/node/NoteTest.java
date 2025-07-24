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

   Source File Name = NoteTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.node;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Node.NodeStatus;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.junit.*;

import static org.junit.Assert.assertTrue;

public class NoteTest {

    private static Sequoiadb sdb;
    private static ReplicaGroup rg = null;
    private static Node node = null;
    private static final String HOST = Constants.NODE_HOST;
    private static final int PORT = Constants.NODE_PORT;
    private static boolean isCluster = true;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        isCluster = Constants.isCluster();
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (!isCluster)
            return;
        // rg
        rg = sdb.getReplicaGroup(Constants.GROUPNAME);
        // node
        node = rg.getNode(HOST, PORT);
    }

    @After
    public void tearDown() throws Exception {
        if (!isCluster)
            return;
    }

    @Test
    public void test() {
        if (!isCluster)
            return;
        assertTrue(0 == 0);
    }

    @Ignore
    @Test
    public void traverseClassNode() {
        if (!isCluster)
            return;
        // getNodeId
        int id = 0;
        id = node.getNodeId();
        assertTrue(id != 0);
        // getShard
        ReplicaGroup s = null;
        s = node.getReplicaGroup();
        assertTrue(s != null);
        // connect
        Sequoiadb connect = null;
        DBCursor cursor = null;
        connect = node.connect();
        cursor = connect.getList(4, null, null, null);
        assertTrue(connect != null);
        assertTrue(cursor != null);
        // disconnect
        connect.disconnect();
        try {
            cursor = connect.getList(4, null, null, null);
        } catch (BaseException e) {
            assertTrue(e.getErrorType().equals("SDB_NETWORK"));
        }
        // getSdb
        Sequoiadb ddb = null;
        ddb = node.getSdb();
        assertTrue(ddb != null);
        // getHostName
        String hostName = null;
        hostName = node.getHostName();
        assertTrue(hostName != null);
        // getHost
        int port = 0;
        port = node.getPort();
        assertTrue(port == PORT);
        // getNodeName
        String nodeName = null;
        nodeName = node.getNodeName();
        System.out.println(nodeName);
        // getStatus
        NodeStatus status = NodeStatus.SDB_NODE_UNKNOWN;
        status = node.getStatus();
        assertTrue(status != NodeStatus.SDB_NODE_UNKNOWN);
    }

}
