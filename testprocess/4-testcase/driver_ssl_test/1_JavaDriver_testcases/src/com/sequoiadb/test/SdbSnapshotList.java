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

   Source File Name = SdbSnapshotList.java

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
import java.util.List;

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.ConfigOptions;


public class SdbSnapshotList {
	
	private static Sequoiadb sdb ;
	private static CollectionSpace cs ;
	private static DBCollection cl ;
	private static DBCursor cursor ;
	private static boolean isCluster = true ;
	
	@BeforeClass
	public static void setConnBeforeClass() throws Exception{
		isCluster = Constants.isCluster();
		// sdb
		ConfigOptions options = new ConfigOptions();
		options.setUseSSL(true);
		sdb = new Sequoiadb(Constants.COOR_NODE_CONN,"","",options);
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
	
	@AfterClass
	public static void DropConnAfterClass() throws Exception {
		sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
		sdb.disconnect();
	}
	
	@Before
	public void setUp() throws Exception {
	}

	@After
	public void tearDown() throws Exception {
	}
	
	@Test
	public void getSnapshot(){
		BSONObject matcher = new BasicBSONObject();
		BSONObject selector = new BasicBSONObject();
		BSONObject orderBy = new BasicBSONObject();
		matcher.put("Name", Constants.TEST_CS_NAME_1+"."+Constants.TEST_CL_NAME_1);
		selector.put("Name", 1);
		selector.put("Details", 1);
		orderBy.put("Details", 1);
		// 4
		cursor = sdb.getSnapshot(4, matcher, selector, orderBy);
	    assertTrue(cursor.hasNext());
//	    System.out.println(cursor.getNext().toString());
	    // 0
	    cursor = sdb.getSnapshot(0, "", "", "");
	    assertTrue(cursor.hasNext());
	    // 1
	    cursor = sdb.getSnapshot(1, "", "", "");
	    assertTrue(cursor.hasNext());
	    // 2
	    cursor = sdb.getSnapshot(2, "", "", "");
	    assertTrue(cursor.hasNext());
	    // 3
	    cursor = sdb.getSnapshot(3, "", "", "");
	    assertTrue(cursor.hasNext());
	    // 5
	    cursor = sdb.getSnapshot(5, "", "", "");
	    assertTrue(cursor.hasNext());
	    // 6
	    cursor = sdb.getSnapshot(6, "", "", "");
	    assertTrue(cursor.hasNext());
	    // 7
	    cursor = sdb.getSnapshot(7, "", "", "");
	    assertTrue(cursor.hasNext());
	    // 8
	    try{
	    	cursor = sdb.getSnapshot(8, "", "", "");
		    assertTrue(cursor.hasNext());
	    }catch(BaseException e){
	    	if(!e.getErrorType().equals("SDB_RTN_COORD_ONLY"))		
	    		assertTrue(false);
	    	return;
	    }
	}
	
	@Test
	public void getList(){
		BSONObject matcher = new BasicBSONObject();
		BSONObject selector = new BasicBSONObject();
		BSONObject orderBy = new BasicBSONObject();
		matcher.put("Status", "Running");
		selector.put("SessionID", 1);
		selector.put("TID", 1);
		selector.put("Status", 1);
		selector.put("Type", 1);
		orderBy.put("TID", 1);
		// 2
		cursor = sdb.getList(2, matcher, selector, orderBy);
	    assertTrue(cursor.hasNext());
		// 0
		cursor = sdb.getList(0, null, null, null);
	    assertTrue(cursor.hasNext());
		// 1
		cursor = sdb.getList(1, null, null, null);
	    assertTrue(cursor.hasNext());
		// 3
		cursor = sdb.getList(3, null, null, null);
	    assertTrue(cursor.hasNext());
		// 4
		cursor = sdb.getList(4, null, null, null);
	    assertTrue(cursor.hasNext());
		// 5
		cursor = sdb.getList(5, null, null, null);
	    assertTrue(cursor.hasNext());
		// 6 in cluster, we need to connect to data node to test,
	    if(!isCluster){
		    cl.insert(new BasicBSONObject("a", 1));
			cursor = sdb.getList(6, null, null, null);
		    assertTrue(cursor.hasNext());
	    }
		// 7
	    try{
	    	cursor = sdb.getList(7, null, null, null);
	    	assertTrue(cursor.hasNext());
	    }catch(BaseException e){
	    	if(!e.getErrorType().equals("SDB_RTN_COORD_ONLY"))
	    		assertTrue(false);
	    }
		// 8
    	if(isCluster)
    	{
		    cursor = null;
			cursor = sdb.getList(8, null, null, null);
		    assertTrue(null != cursor);
    	}

	    // 9
	    try {
		    cursor = null;
			cursor = sdb.getList(9, null, null, null);
		    assertTrue(null != cursor);
	    }catch(BaseException e){
	    	if(!e.getErrorType().equals("SDB_RTN_COORD_ONLY"))
	    		assertTrue(false);
	    }
	    // 10
	    if (isCluster)
	    {
		    cursor = null;
			cursor = sdb.getList(10, null, null, null);
		    assertTrue(null != cursor);
	    }
	}
}
