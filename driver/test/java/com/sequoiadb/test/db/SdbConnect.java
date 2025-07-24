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

   Source File Name = SdbConnect.java

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

import static org.junit.Assert.*;
import static org.junit.Assert.assertEquals;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.test.common.*;

public class SdbConnect {
	
	private static Sequoiadb sdb ;
	private static CollectionSpace cs ;
	private static DBCollection cl ;
	private static ReplicaGroup rg ;
	private static Node node ;
	private static DBCursor cursor ;
	
	@BeforeClass
	public static void setConnBeforeClass() throws Exception{
		// sdb
		sdb = new Sequoiadb(Constants.COOR_NODE_CONN,"","");
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
	public void sdbConnect() {
		List<String> list = new ArrayList<String>();
		try {
			list.add("192.168.20.35:12340");
			list.add("192.168.20.36:12340");
			list.add("123:123");
			list.add("");
			list.add(":12340");
			list.add("localhost:50000");
			list.add("localhost:11810");
			list.add("localhost:12340");
			list.add(Constants.COOR_NODE_CONN);
	
			ConfigOptions options = new ConfigOptions();
			options.setMaxAutoConnectRetryTime(0);
			options.setConnectTimeout(10000);
			// connect
			long begin = 0;
			long end = 0;
			begin = System.currentTimeMillis();
			Sequoiadb sdb1 = new Sequoiadb(list, "", "", options);
			end = System.currentTimeMillis();
			System.out.println("Takes " + (end - begin));
			// set option and change the connect
			options.setConnectTimeout(15000);
			sdb1.changeConnectionOptions(options);
			// check
			DBCursor cursor = sdb1.getList(4, null, null, null);
			assertTrue(cursor != null);
			sdb1.disconnect();
		}catch(BaseException e) {
			SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
			System.out.println("Debug info: failed to testcase at: " + df.format(new Date()));
			System.out.println("Debug info: address list is: " + list.toString());
			throw e;
		}
	}
	
	
}
