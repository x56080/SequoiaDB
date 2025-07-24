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

   Source File Name = SDBGetRG.java

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

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.junit.*;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class SDBGetRG {
    private static Sequoiadb sdb;
    private static ReplicaGroup rg;
    private static Node node;
    private static int groupID;
    private static String groupName;
    private static String Name;
    private static boolean isCluster = true;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        isCluster = Constants.isCluster();
        if (!isCluster)
            return;
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        rg = sdb.getReplicaGroup(1000);
        Name = rg.getGroupName();
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        if (!isCluster)
            return;
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (!isCluster)
            return;
        groupID = Constants.GROUPID;
        groupName = Name;
    }

    @After
    public void tearDown() throws Exception {
        if (!isCluster)
            return;
    }

    @Test
    public void getGroupTest() {
        if (!isCluster) {
            return;
        }

        boolean groupExist = sdb.isRelicaGroupExist("SYSCatalogGroup");
        Assert.assertTrue(groupExist);
        groupName = "SYSCatalogGroup12345";
        groupExist = sdb.isRelicaGroupExist(groupName);
        Assert.assertFalse(groupExist);
        try {
            rg = sdb.getReplicaGroup(groupName);
            Assert.fail("should get SDB_CLS_GRP_NOT_EXIST(-154) error");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_CLS_GRP_NOT_EXIST.getErrorCode(), e.getErrorCode());
        }
        groupExist = sdb.isReplicaGroupExist(1);
        Assert.assertTrue(groupExist);
        groupExist = sdb.isReplicaGroupExist(0);
        Assert.assertFalse(groupExist);
        try {
            rg = sdb.getReplicaGroup(0);
            Assert.fail("should get SDB_CLS_GRP_NOT_EXIST(-154) error");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_CLS_GRP_NOT_EXIST.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void getNodeTest() {
        if (!isCluster) {
            return;
        }
        // case 1: normal case
        groupName = "SYSCatalogGroup";
        rg = sdb.getReplicaGroup(groupName);
        Node master = rg.getMaster();
//        System.out.println(String.format("group is: %s, master is: %s", groupName, master.getNodeName()));
        String hostName = master.getHostName();
        int hostPort = master.getPort();
        boolean nodeExist = rg.isNodeExist(hostName + ":" + hostPort);
        Assert.assertTrue(nodeExist);
        nodeExist = rg.isNodeExist(hostName, hostPort);
        Assert.assertTrue(nodeExist);
        Node node1 = rg.getNode(hostName, hostPort);
//        System.out.println(String.format("group is: %s, node1 is: %s", groupName, node1.getNodeName()));
        Node node2 = rg.getNode(hostName + ":" + hostPort);
//        System.out.println(String.format("group is: %s, node2 is: %s", groupName, node2.getNodeName()));
        // case 2: get a node which is not exist
        Node node3 = null;
        nodeExist = rg.isNodeExist("ubuntu" + ":" + 30000);
        Assert.assertFalse(nodeExist);
        nodeExist = rg.isNodeExist("ubuntu", 30000);
        Assert.assertFalse(nodeExist);
        try {
            node3 = rg.getNode("ubuntu", 30000);
            Assert.fail("should get SDB_CLS_NODE_NOT_EXIST(-155) error");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_CLS_NODE_NOT_EXIST.getErrorCode(), e.getErrorCode());
        }
        try {
            node3 = rg.getNode(hostName, 0);
            Assert.fail("should get SDB_CLS_NODE_NOT_EXIST(-155) error");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_CLS_NODE_NOT_EXIST.getErrorCode(), e.getErrorCode());
        }
        // case 3: get a node from empty group
        groupName = "groupNoteExist";
        rg = sdb.createReplicaGroup(groupName);
        try {
            node3 = rg.getNode(hostName, 0);
            Assert.fail("should get SDB_CLS_NODE_NOT_EXIST(-155) error");
        } catch (BaseException e) {
            Assert.assertEquals("SDB_CLS_NODE_NOT_EXIST", e.getErrorType());
        } finally {
            sdb.removeReplicaGroup(groupName);
        }

    }

    @Test
    public void getMasterAndSlaveNodeTest() {
        if (!isCluster)
            return;
        groupName = "SYSCatalogGroup";
        rg = sdb.getReplicaGroup(groupName);
        Node master = rg.getMaster();
        Node slave = rg.getSlave();
        System.out.println(String.format("group is: %s, master is: %s, slave is: %s", groupName, master.getNodeName(), slave.getNodeName()));
    }
    
    @Test
    public void getReplicaGroupById() {
        if (!isCluster)
            return;
        rg = sdb.getReplicaGroup(groupID);
        int id = rg.getId();
        assertEquals(groupID, id);
    }

    @Test
    public void getReplicaGroupByName() {
        if (!isCluster)
            return;
        groupName = Constants.CATALOGRGNAME;
        rg = sdb.getReplicaGroup(groupName);
        String name = rg.getGroupName();
        assertEquals(groupName, name);
    }

    @Test
    public void getCataReplicaGroupById() {
        if (!isCluster)
            return;
        groupID = 1;
        rg = sdb.getReplicaGroup(groupID);
        int id = rg.getId();
        assertEquals(groupID, id);
        boolean f = rg.isCatalog();
        assertTrue(f);
    }

    @Test
    public void getCataReplicaGroupByName() {
        if (!isCluster)
            return;
        groupName = Constants.CATALOGRGNAME;
        rg = sdb.getReplicaGroup(groupName);
        String name = rg.getGroupName();
        assertEquals(groupName, name);
        boolean f = rg.isCatalog();
        assertTrue(f);
    }

    @Test
    public void getCataReplicaGroupByName1() {
        if (!isCluster)
            return;
        try {
            // no replica group can name start wtih "SYS"
            groupName = "SYSCatalogGroupForTest";
            rg = sdb.createReplicaGroup(groupName);
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(),
                e.getErrorCode());
            return;
        }
        assertTrue(false);
    }

}
