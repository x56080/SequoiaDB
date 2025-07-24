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

   Source File Name = SdbSetSessionAttr.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test;

import static org.junit.Assert.*;
import static org.junit.Assert.assertEquals;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.ConfigOptions;


public class SdbSetSessionAttr {
	
	private static Sequoiadb sdb ;
	private static CollectionSpace cs ;
	private static DBCollection cl ;
	private static ReplicaGroup rg ;
	private static Node node ;
	private static DBCursor cursor ;
	private static boolean isCluster = true ;
	
	/*
	@BeforeClass
	public static void setConnBeforeClass() throws Exception{
		isCluster = Constants.isCluster();
		try{
			// sdb
			sdb = new Sequoiadb(Constants.COOR_NODE_CONN,"","");
			// todo:
			BSONObject conf = new BasicBSONObject();
			conf.put("PreferedReplica", 3);
			sdb.setSessionAttr(conf);
			// create another node
			shard = sdb.getShard("group1");
			node = shard.createNode("ubuntu-dev1", Constants.SERVER3,
					                Constants.DATAPATH3,
					                new HashMap<String, String>());
			node.start();
		} catch (BaseException e){
			System.out.println(e.getMessage());
			e.printStackTrace();
			return;
		}
	}
	
	@AfterClass
	public static void DropConnAfterClass() throws Exception {
		try{
			node.stop();
			shard.removeNode("ubuntu-dev1", Constants.SERVER3, null);
			sdb.disconnect();
		} catch (BaseException e){
			System.out.println(e.getMessage());
			e.printStackTrace();
			return;
		}
	}
	
	@Before
	public void setUp() throws Exception {
		// cs
		if(sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)){
			sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
			cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
		}
		else
			cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
		// cl
		BSONObject conf = new BasicBSONObject();
		conf.put("ReplSize", 0);
		cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
	}

	@After
	public void tearDown() throws Exception {
		sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
	}

	@Test
	public void setSessionAttr(){
		if(!isCluster)
			return;
		// insert one record
		BSONObject record = new BasicBSONObject();
		record.put("a", 1);
		cl.insert(record);
		// todo:
		BSONObject conf = new BasicBSONObject();
		conf.put("PreferedReplica", "s");
		sdb.setSessionAttr(conf);
		// check
		long num1 = 0;
		long num2 = 0;
	    final int num = 10;
		Sequoiadb ddb = new Sequoiadb("ubuntu-dev1", Constants.SERVER3, "", "");
		BSONObject selector = new BasicBSONObject("TotalRead", "");
		cursor = ddb.getSnapshot(6, null, selector, null);
		while(cursor.hasNext()) {
			num1 = (Long)cursor.getNext().get("TotalRead");
			break;
		}
		for( int i = 0; i < num; i++ ){
			cl.query();
		}
		cursor = ddb.getSnapshot(6, null, selector, null);
		while(cursor.hasNext()) {
			num2 = (Long)cursor.getNext().get("TotalRead");
			break;
		}
		System.out.println("num = " + num + " , num1 = " + num1 + ", num2 = " + num2);
		assertTrue( num <= (num2 - num1) );
	}
	*/
	
	@BeforeClass
	public static void setConnBeforeClass() throws Exception{
		isCluster = Constants.isCluster();
		// sdb
		ConfigOptions options = new ConfigOptions();
		options.setUseSSL(true);
		sdb = new Sequoiadb(Constants.COOR_NODE_CONN,"","",options);
	}
	
	@AfterClass
	public static void DropConnAfterClass() throws Exception {
		sdb.disconnect();
	}
	
	@Before
	public void setUp() throws Exception {
		// cs
		if(sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)){
			sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
			cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
		}
		else
			cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
		// cl
		BSONObject conf = new BasicBSONObject();
		conf.put("ReplSize", 0);
		cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
	}

	@After
	public void tearDown() throws Exception {
		sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
	}
	
	@Test
	public void setSessionAttr_test_arguments(){
		if(!isCluster)
			return;
		String[] str = {"M", "m", "S", "s", "A", "a"};
		int[] in = {1, 2, 3, 4, 5, 6, 7};
		Random random = new Random();
		int r1 = random.nextInt(str.length);
		int r2 = random.nextInt(in.length);
		BSONObject conf = new BasicBSONObject();
		if (random.nextInt(2) == 0) {
			System.out.println("r1 is " + r1);
			conf.put("PreferedInstance", str[r1]);
		}
		else {
			System.out.println("r2 is " + r2);
			conf.put("PreferedInstance", in[r2]);
		}
		// test
		try{
			sdb.setSessionAttr(conf);
		}catch(BaseException e){
			System.out.println(e.getMessage());
			assertTrue(false);
		}
		final int num = 10; 
		for(int i = 0; i < num; i++){
			cl.query();
		}
	}
}
