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

import static org.junit.Assert.*;

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.*;

public class SDBGetRG {
	private static Sequoiadb sdb ;
	private static ReplicaGroup rg ;
	private static Node node ;
	private static int groupID ;
	private static String groupName ;
	private static String Name ;
	private static boolean isCluster = true;
	
	@BeforeClass
	public static void setConnBeforeClass() throws Exception{
		// sdb
		isCluster = Constants.isCluster();
		if(!isCluster)
			return;
		sdb = new Sequoiadb(Constants.COOR_NODE_CONN,"","");
		rg = sdb.getReplicaGroup(1000);
		Name = rg.getGroupName();
	}
	
	@AfterClass
	public static void DropConnAfterClass() throws Exception {
		if(!isCluster)
			return;
		sdb.disconnect();
	}
	
	@Before
	public void setUp() throws Exception {
		if(!isCluster)
			return;
		groupID = Constants.GROUPID ;
		groupName = Name ;
	}

	@After
	public void tearDown() throws Exception {
		if(!isCluster)
			return;
	}
	
	@Test
	public void getReplicaGroupById(){
		if(!isCluster)
			return;
		rg = sdb.getReplicaGroup(groupID);
		int id = rg.getId();
	    assertEquals(groupID, id);
	}
	
	@Test
	public void getReplicaGroupByName(){
		if(!isCluster)
			return;
		groupName = Constants.CATALOGRGNAME ;
		rg = sdb.getReplicaGroup(groupName);
		String name = rg.getGroupName();
	    assertEquals(groupName, name);
	}
	
	@Test
	public void getCataReplicaGroupById(){
		if(!isCluster)
			return;
        groupID = 1 ;
		rg = sdb.getReplicaGroup(groupID);
		int id = rg.getId();
	    assertEquals(groupID, id);
	    boolean f = rg.isCatalog();
	    assertTrue( f );
	}
	
	@Test
	public void getCataReplicaGroupByName(){
		if(!isCluster)
			return;
		groupName = Constants.CATALOGRGNAME ;
		rg = sdb.getReplicaGroup(groupName);
		String name = rg.getGroupName();
	    assertEquals(groupName, name);
	    boolean f = rg.isCatalog();
	    assertTrue( f );
	}
	
	@Test
	public void getCataReplicaGroupByName1(){
		if(!isCluster)
			return;
		try {
			// no replica group can name start wtih "SYS"
			groupName = "SYSCatalogGroupForTest" ;
			rg = sdb.createReplicaGroup(groupName);
		}catch(BaseException e){
			assertEquals(new BaseException("SDB_INVALIDARG").getErrorCode(),
					     e.getErrorCode());
			return ;
		}
		assertTrue(false);
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
  @Ignore
  public void groupTmpTest() {
      if (!isCluster)
          return;
      groupName = "SYSCatalogGroup";
      rg = sdb.getReplicaGroup(groupName);
      BSONObject detail = rg.getDetail();
      BasicBSONList nodeList = (BasicBSONList) detail.get("Group");
      int nodeCount = nodeList.size();
//      int primaryNodePosition = getMasterPosition(rg);
      //assertTrue(nodeCount != 0);

      Node master = null;
      Node slave = null;

      // case 1
      master = rg.getMaster();
      slave = rg.getSlave();
      System.out.println(String.format("case1: group is: %s, master is: %s, slave is: %s", groupName,
              master == null ? null : master.getNodeName(),
              slave == null ? null : slave.getNodeName()));
      int counter1 = 0, counter2 = 0;
      String str1 = "susetzb:40000", str2 = "susetzb:42000";
      for(int i = 0; i < 100; i++) {
//          slave = rg.getSlave(1,2,3,4,5,6,7);
          slave = rg.getSlave();
          if (str1.equals(slave.getNodeName())) {
              counter1++;
          } else if(str2.equals(slave.getNodeName())) {
              counter2++;
          }
      }
      System.out.println("counter1 is: " + counter1 + ", counter2 is: " + counter2);
  }
}
