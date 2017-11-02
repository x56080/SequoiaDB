package com.sequoiadb.test.db;

import com.sequoiadb.base.*;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

public class SdbTest {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static ReplicaGroup rg;
    private static Node node;
    private static DBCursor cursor;

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
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
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
    public void sdbisValid() {
    }
//	@Test
//	public void sdbisValid(){
//		Sequoiadb conn = null;
//		DBCursor cur = null;
//		boolean result = false ;
//		conn = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
//		cur = conn.listCollections();
//		assertTrue(cur != null);
//		// TODO:
//		// case 1: the connection is valid
//		result = conn.isValid();
//		assertTrue(result == true);
//		System.out.println("before disconnect from database, result is " + result);
//		
//		/*
//		// case 2(manually): disconnect from server, we can get a packet come from server
//		result = conn.isValid();
//		assertTrue(result == false);
//		
//		// case 3(manually): network error, we can't get any message from server
//		*/
//		// case 4 : disconnect by client
//		conn.disconnect();
//		result = conn.isValid();
//		assertTrue(result == false);
//		System.out.println("after disconnect from database, result is " + result);
//		
//		System.out.println("ok.");
//	}
}
