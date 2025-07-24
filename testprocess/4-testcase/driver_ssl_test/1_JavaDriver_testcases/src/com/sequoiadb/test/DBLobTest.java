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

   Source File Name = DBLobTest.java

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

import static org.junit.Assert.assertEquals;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.testdata.SDBTestHelper;

public class DBLobTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    
    private static final String LOB_SIZE = "Size";
    private static final String LOB_AVAILABLE = "Available";
    private static final String LOB_OID = "Oid";

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {

    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

    }

    @Before
    public void setUp() throws Exception {
        // sdb
    	ConfigOptions options = new ConfigOptions();
		options.setUseSSL(true);
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "admin", "admin",options);
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        else {
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }
    
    /*
     * create an empty lob
     * */
    @Test
    public void testCreateLob() throws BaseException {
        DBLob lob = cl.createLob();
        lob.close();
        
        long createTime = lob.getCreateTime();
        long size       = lob.getSize();
        ObjectId id     = lob.getID();
        SDBTestHelper.println( "id:" + id );
        SDBTestHelper.println( "createTime:" + SDBTestHelper.millisToDate(createTime) );
        SDBTestHelper.println( "lobSize:" + size );
        assertEquals( 0, size );
    }
    
    /*
     * test create Lob with ID
     * */
    @Test
    public void testCreateLobWithID() throws BaseException {
        ObjectId id = ObjectId.get();
        DBLob lob   = cl.createLob(id);
        String w = "Helloworld";
        lob.write(w.getBytes());
        lob.close();
        
        lob = cl.openLob(id);
        byte read[] = new byte[100];
        int len = lob.read(read);
        String s = new String(read, 0, len);
        SDBTestHelper.println("read data:" + s);
        assertEquals(true, w.equals(s));
    }
    
    @Test
    public void testCreateWriteLob() throws BaseException {
        // create a lob and write data into this lob
        DBLob lob = cl.createLob();
        String data = new String( "HelloWorld1234567890" );
        lob.write(data.getBytes());
        lob.close();
        
        ObjectId id     = lob.getID();
        
        // read data from the lob just created.
        DBLob rLob = cl.openLob(id);
        
        byte[] b = new byte[5];
        int len = rLob.read(b);
        assertEquals( 5, len );
        String rData = new String( b, 0, len );
        assertEquals( true, rData.equals( data.substring( 0, 5 ) ) );
        rLob.close();
    }
    
    @Test
    public void testSeekLob() throws BaseException {
        // create a lob and write data into this lob
        DBLob lob = cl.createLob();
        // data.length = 10(this data affect the the follow code)
        String data = new String( "HelloWorld1234567890" );
        lob.write(data.getBytes());
        lob.close();
        
        ObjectId id     = lob.getID();
        
        // read data from the lob just created.
        DBLob rLob = cl.openLob(id);
        
        // offset = 5
        rLob.seek(5, DBLob.SDB_LOB_SEEK_SET);
        byte[] b = new byte[5];
        // after read offset = 10
        int len = rLob.read(b);
        assertEquals( 5, len );
        String rData = new String( b, 0, len );
        assertEquals( true, rData.equals( data.substring( 5, 10 ) ) );
        
        // now we change offset = 0
        rLob.seek(-10, DBLob.SDB_LOB_SEEK_CUR);
        // after read offset = 5
        len = rLob.read(b);
        assertEquals( 5, len );
        rData = new String( b, 0, len );
        assertEquals( true, rData.equals( data.substring( 0, 5 ) ) );
        
        // now we change offset = 15
        rLob.seek(5, DBLob.SDB_LOB_SEEK_END);
        // after read offset = 5
        len = rLob.read(b);
        assertEquals( 5, len );
        rData = new String( b, 0, len );
        assertEquals( true, rData.equals( data.substring( 15, 20 ) ) );
        
        rLob.close();
    }
    
    @Test
    public void testListLobs() throws BaseException {
        // create two lob
        DBLob lob = cl.createLob();
        lob.close();
        
        ObjectId id1 = lob.getID();
        
        lob = cl.createLob();
        lob.write("123".getBytes());
        ObjectId id2 = lob.getID();
        lob.close();
        
        DBCursor cur = cl.listLobs();
        int count = 0;
        while ( cur.hasNext() ) {
            BSONObject obj = cur.getNext();
            SDBTestHelper.println(obj.toString());
            ObjectId id = (ObjectId) obj.get(LOB_OID);
            assertEquals(true, id.equals(id1) || id.equals(id2));
             
            if ( id.equals( id1 ) ) {
                long size           = (Long) obj.get(LOB_SIZE);
                boolean isAvailable = (Boolean) obj.get(LOB_AVAILABLE);
                assertEquals( true, isAvailable );
                assertEquals( 0, size );
            }
            else {
                long size           = (Long) obj.get(LOB_SIZE);
                boolean isAvailable = (Boolean) obj.get(LOB_AVAILABLE);
                assertEquals( true, isAvailable );
                assertEquals( 3, size );
            }
            
            count++;
        }
        
        assertEquals(2, count);
    }
    
    @Test
    public void testRemoveLob() throws BaseException {
        // create two lob
        DBLob lob = cl.createLob();
        lob.close();
        
        ObjectId id1 = lob.getID();
        
        lob = cl.createLob();
        lob.write("123".getBytes());
        ObjectId id2 = lob.getID();
        lob.close();
        
        // remove id1's lob
        cl.removeLob(id1);
        
        DBCursor cur = cl.listLobs();
        while ( cur.hasNext() ) {
            BSONObject obj = cur.getNext();
            SDBTestHelper.println(obj.toString());
            ObjectId id = (ObjectId) obj.get(LOB_OID);
            assertEquals(true, id.equals(id2));
             
            long size           = (Long) obj.get(LOB_SIZE);
            boolean isAvailable = (Boolean) obj.get(LOB_AVAILABLE);
            assertEquals( true, isAvailable );
            assertEquals( 3, size );
        }
    }
    
    @Test
    public void testLargeFile(){
		
		//1024 bits
		String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
		//26 bits
		String s2 = "abcdefghijklmnopqrstuvwxyz";
		//times
		int t1 = 1024*16;
		int t2 = 1;
		long allSize = s1.getBytes().length * t1 + s2.getBytes().length * t2;
		System.out.println("allSize:" + allSize);
		
		ObjectId id = ObjectId.get();
		DBLob lob   = cl.createLob(id);
		
		//insert lob
		for(int i=0; i<t1; i++){
			lob.write(s1.getBytes());
		}
		for(int i=0; i<t2; i++){
			lob.write(s2.getBytes());
		}
		lob.close();
		
		//list lob
		DBCursor cur = cl.listLobs();
		while(cur.hasNext()){
			BSONObject obj = cur.getNext();
			ObjectId cid = (ObjectId) obj.get(LOB_OID);
			if(id.equals(cid)){
				long size  = (Long) obj.get(LOB_SIZE);
				boolean isAvailable = (Boolean) obj.get(LOB_AVAILABLE);
				assertEquals( true, isAvailable );
				assertEquals( allSize, size );
				break;
			}
		}
		cur.close();
		
		//read lob
		DBLob rLob = cl.openLob(id);
		int len = 0;
		long total = 0;
		byte[] tmp = new byte[1000];
		while((len = rLob.read(tmp)) > 0){
			total += len;
			if(total>allSize)
				break;
		}
		assertEquals( allSize, total );
		rLob.close();
		//remove lob
		cl.removeLob(id);
	}
	
	@Test
	public void testWithoutClose(){
		int times = 1000;
		
		//1024 bits
		String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
		byte[] tmp = new byte[s1.getBytes().length + 1];
		
		ObjectId id = ObjectId.get();
		
		DBLob lob   = cl.createLob(id);
		lob.write(s1.getBytes());
		lob.close();
		
		for(int i=0; i<times; i++){
			DBLob rLob = cl.openLob(id);
			int len = rLob.read(tmp);
			if(len != s1.getBytes().length)
			{
				System.out.println("open times:" + i);
				assertEquals( s1.getBytes().length, len );
				break;
			}
		}
		
		//remove lob
		cl.removeLob(id);
	}
	
	@Test
	public void testLobFalse(){
		//1024 bits
		String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
		byte[] tmp = new byte[s1.getBytes().length + 1];
		
		ObjectId id = ObjectId.get();
		
		//insert lob
		DBLob lob   = cl.createLob(id);
		lob.write(s1.getBytes());
		//without close
		//lob.close();
		
		try{
			DBLob rLob = cl.openLob(id);
			rLob.read(tmp);
			rLob.close();
			System.err.println("error:writing a lob without calling close method but the lob can be read!");
			assertEquals( true, false );
		}catch(BaseException e){
			System.out.println("writing a lob without calling close method and the lob can't be read");
		}
		
		//close lob
		lob.close();
		
		try{
			DBLob rLob = cl.openLob(id);
			rLob.read(tmp);
			rLob.close();
			System.out.println("writing a lob with calling close method and the lob can be read");
		}catch(BaseException e){
			System.err.println("error:writing a lob with calling close method but the lob can't be read");
			assertEquals( true, false );
		}

		//remove lob
		cl.removeLob(id);
	}
	
	@Test
    public void testOpenSameLobID(){
		//1024 bits
		String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
		byte[] tmp = new byte[s1.getBytes().length + 1];
		
		ObjectId id = ObjectId.get();
		
		//insert lob
		DBLob lob   = cl.createLob(id);
		lob.write(s1.getBytes());
		lob.close();
		
		int nums = 100;
		DBLob[] rlobs = new DBLob[nums];
		
		//open lob
		for(int i=0; i<nums; i++){
			rlobs[i] = cl.openLob(id);
		}
		
		//read lob
		for(int i=0; i<nums; i++){
			int len = rlobs[i].read(tmp);
			if(len != s1.getBytes().length){
				assertEquals( s1.getBytes().length, len );
				System.out.println("error: read lob length is not expected at " + i + " times!");
				System.out.println("expected:" + s1.getBytes().length + "\n returned:" + len);
				break;
			}
		}
		
		//close
		for(int i=0; i<nums; i++){
			rlobs[i].close();
		}
		
		//remove lob
		cl.removeLob(id);
	}
	
	@Test
  public void testRemoveFalseLob(){
		//1024 bits
		String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
		
		ObjectId id = ObjectId.get();
		
		//insert lob
		DBLob lob   = cl.createLob(id);
		lob.write(s1.getBytes());
		//without close
		//lob.close();
		
		try {
			//remove lob
			cl.removeLob(id);
			assertEquals( true, false );
			System.err.println("error:remove unavailable lob, expected false, but return true!");
		}catch(BaseException e){
			System.out.println("can't remove unavailable lob");
		}
		
		//close lob
		lob.close();
		
		cl.removeLob(id);
	}
	
	@Test
  public void testWriteAndDisconnect(){
		//1024 bits
		String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
		byte[] tmp = new byte[s1.getBytes().length + 1];
		
		ObjectId id = ObjectId.get();
		
		//insert lob
		DBLob lob   = cl.createLob(id);
		lob.write(s1.getBytes());
		//without close
		//lob.close();
		
		sdb.disconnect();
		
		sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "admin", "admin");
		cs = sdb.getCollectionSpace(Constants.TEST_CS_NAME_1);
		cl = cs.getCollection(Constants.TEST_CL_NAME_1);
		
		try{
			lob.close();
			System.err.println("error:sdb is disconnected but lob.close succ!");
		}catch(BaseException e){
			System.out.println("");
		}
		
		//list lob
		DBCursor cur = cl.listLobs();
		while(cur.hasNext()){
			BSONObject obj = cur.getNext();
			ObjectId cid = (ObjectId) obj.get(LOB_OID);
			if(id.equals(cid)){
				long size  = (Long) obj.get(LOB_SIZE);
				boolean isAvailable = (Boolean) obj.get(LOB_AVAILABLE);
				assertEquals( false, isAvailable );
				assertEquals( 0, size );
			}
		}
		cur.close();
		
		//read lob
		try{
			DBLob rLob = cl.openLob(id);
			rLob.read(tmp);
			rLob.close();
			assertEquals( true, false );
			System.err.println("error:disconnect before calling close method but the lob can be read");
		}catch(BaseException e){
			System.out.println("disconnect before calling close method and the lob can't be read");
		}
		
		//remove lob
		try {
			//remove lob
			cl.removeLob(id);
			assertEquals( true, false );
			System.err.println("error:remove unavailable lob, expected false, but return true!");
		}catch(BaseException e){
			System.out.println("can't remove unavailable lob");
		}
	}
}
