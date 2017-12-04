package com.sequoiadb.lob;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import java.nio.ByteBuffer;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.BeforeClass;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
* FileName: TestSameLobs7841.java
* test content:write the same lob
* testlink case:seqDB-7841
* @author wuyan
    * @Date    2016.9.12
* @version 1.00
*/
public class TestSameLobs7841 extends SdbTestBase {
	
	private String clName = "cl_lob7841";
	private Sequoiadb sdb = null;
	private CollectionSpace cs = null;
	private DBCollection cl = null;
	private Random random = new Random();	
	private String prevMd5 = "";    
	
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
	
	private ObjectId putLob(){
		int lobsize = random.nextInt(1048576);
		String lobSb = LobUtils.getRandomString(lobsize);
		ObjectId oid  = null;			
		
		DBLob lob = null;
		try{			
			lob = cl.createLob();
			lob.write(lobSb.getBytes());
		
			prevMd5 = LobUtils.getMd5(lobSb);
		    oid = lob.getID();		    
		}catch(BaseException e){	
			Assert.assertTrue(false,"write lob fail:"+e.getMessage()+e.getStackTrace());
		}finally{
			if (lob != null){
				lob.close();
			}
		}
		return oid;
		
	}
		
	private void checkLob(ObjectId oid){
		String curMd5 ="";
		DBLob rLob = null;
		try
		{
			rLob = cl.openLob(oid);
			
			int rbuffSize = 1024;
			byte[] rbuff = new byte[rbuffSize];
			int readLen =0;
		
			ByteBuffer bytebuff = ByteBuffer.allocate((int)rLob.getSize());
			while ((readLen = rLob.read(rbuff)) != -1){
				bytebuff.put(rbuff, 0, readLen);				
			}
			bytebuff.rewind();		
			curMd5 = LobUtils.getMd5(bytebuff);
			Assert.assertEquals(curMd5, prevMd5,"the lobs md5 different");
		}catch(BaseException e){
			Assert.assertTrue(false,"read lob fail:"+e.getMessage()+e.getStackTrace());
		}finally{
			if (rLob != null){
				rLob.close();
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
	
	//write the 100 same lob 
	@Test(invocationCount = 100)
	private void testSameLob(){		
		ObjectId oid = putLob();
		checkLob(oid);
	}	
	
	
}

