package com.sequoiadb.lob;

import java.nio.ByteBuffer;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;

import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
* FileName: TestReadAndRemoveLobs7843.java
* test content:when delete lob of reading 
* testlink case:seqDB-7843
* @author wuyan
    * @Date    2016.9.12
* @version 1.00
*/
public class TestReadAndRemoveLobs7843 extends SdbTestBase {
	
	private String clName = "cl_lob7843";	
	private static Sequoiadb sdb = null;
	private CollectionSpace cs = null;
	private DBCollection cl = null;
	private static ObjectId oid = null;	
	private String prevMd5 = ""; 
	private Random random = new Random();	
    	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+coordUrl+e.getMessage());
		}
		createCL();
	}		
		

	public void createCL(){
		try{
			if (!sdb.isCollectionSpaceExist(SdbTestBase.csName)){
				sdb.createCollectionSpace(SdbTestBase.csName);	
			}
		}catch(BaseException e){
			//-33 CS exist,ignore exceptions
			Assert.assertEquals(-33,e.getErrorCode(),e.getMessage());
	    }					
	    try
	    {
	    	String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
				+ "ReplSize:0,Compressed:true}";
	    	BSONObject options =(BSONObject) JSON.parse(clOptions);
	    	
		    cs = sdb.getCollectionSpace(SdbTestBase.csName);			
		    cl = cs.createCollection(clName,options);			
	    }catch(BaseException e){
		    Assert.assertTrue(false,"create cl fail "+e.getErrorType()+":"+e.getMessage());
	    }
	 }	
	
	
	
	
	public void readLob(DBCollection newCL){
		DBLob rLob = null;	
		try
		{   
			String curMd5 ="";
			rLob = newCL.openLob(oid);			
			byte[] rbuff = new byte[1024];
			int readLen =0;		    
			ByteBuffer bytebuff = ByteBuffer.allocate((int)rLob.getSize());
			while ((readLen = rLob.read(rbuff)) != -1){
				bytebuff.put(rbuff, 0, readLen);
			}
			bytebuff.rewind();	
			rLob.close();
			curMd5 = LobUtils.getMd5(bytebuff);
			Assert.assertEquals(curMd5, prevMd5,"the lobs md5 different");
		}catch(BaseException e){			
			if(-4 != e.getErrorCode() && -269 != e.getErrorCode() && -268 != e.getErrorCode()){
				Assert.assertTrue(false,"removeLob fail:"+e.getMessage()+e.getErrorCode());
			}	
		}finally{
			if (rLob != null){
				rLob.close();
			}
		}
	}

	public void removeLob(DBCollection newCL) {			
		try{
			newCL.removeLob(oid);
		    boolean IsNotExist = true;
			DBCursor cur1 = cl.listLobs();	
			while(cur1.hasNext()){
				BasicBSONObject obj = (BasicBSONObject)cur1.getNext();
				if (obj.getObjectId("Oid").equals(oid)){
					IsNotExist = false;
					break;
				}
			}
			Assert.assertTrue(IsNotExist,"lob remove fail");
		}catch(BaseException e){			
			if( -4 != e.getErrorCode() && -269 != e.getErrorCode() && -268 != e.getErrorCode()){
				Assert.assertTrue(false,"removeLob fail:"+e.getMessage()+e.getErrorCode());
			}			
		}	
	}

	@AfterClass
	public void tearDown(){		
		try{			
			if(cs.isCollectionExist(clName)){
				cs.dropCollection(clName);
			}
			sdb.disconnect();
		}catch(BaseException e){			
			Assert.assertTrue(false,"clean up failed:"+e.getMessage());
		}finally{
		}
	}	
	
	@Test
	private void putLob(){
		int lobsize = random.nextInt(1048576);		
		String lobSb = LobUtils.getRandomString(lobsize);		
		DBLob lob = null;
		try{
			lob = cl.createLob();
			lob.write(lobSb.getBytes());
			oid = lob.getID();
			prevMd5 = LobUtils.getMd5(lobSb);
		}catch(BaseException e){			
			Assert.assertTrue(false,"write lob fail"+e.getMessage());
		}finally{
			if (lob != null){
				lob.close();
			}
		}				
	}
	
	@Test(invocationCount = 4, threadPoolSize = 4, dependsOnMethods="putLob")
	public void testRead(){
		Sequoiadb db  = null;
		try{			
			db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			CollectionSpace newCS = db.getCollectionSpace(SdbTestBase.csName);
			DBCollection newCL = newCS.getCollection(clName);			
			readLob(newCL);
			removeLob(newCL);			
		}catch(BaseException e){
			Assert.assertTrue(false,e.getErrorType()+e.getMessage());			
		}finally{
			if (db != null){
				db.disconnect();
			}
		}		
   }
	
}

 

