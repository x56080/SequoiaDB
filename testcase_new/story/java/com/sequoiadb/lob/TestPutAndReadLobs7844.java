package com.sequoiadb.lob;

import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;
import java.util.concurrent.ConcurrentHashMap;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
* FileName: TestPutAndReadLobs7844.java
* test content:when write lob of reading 
* testlink case:seqDB-7844
* @author wuyan
    * @Date    2016.9.12
* @version 1.00
*/
public class TestPutAndReadLobs7844 extends SdbTestBase {
	private String clName = "cl_lob7844";	
	private static Sequoiadb sdb = null;
	private CollectionSpace cs = null;
	private Random random = new Random();
	private ConcurrentHashMap<ObjectId, String> id2md5 
	                     = new ConcurrentHashMap<ObjectId, String>();
	
    	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+SdbTestBase.coordUrl+e.getMessage());
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
		    cs.createCollection(clName,options);			
	    }catch(BaseException e){
		    Assert.assertTrue(false,"create cl fail "+e.getErrorType()+":"+e.getMessage());
	    }
	 }		
	
	//put lob
	private ObjectId putLob(DBCollection cl){
		int lobsize = random.nextInt(1048576);		
		String lobStringBuff = LobUtils.getRandomString(lobsize);		
		DBLob lob = null;
		ObjectId oid = null;		
		try{
			
			lob = cl.createLob();
			lob.write(lobStringBuff.getBytes());
		
			String prevMd5 = LobUtils.getMd5(lobStringBuff);
		    oid = lob.getID();
		    id2md5.put(oid, prevMd5);
		}catch(BaseException e){			
			Assert.assertTrue(false,"write lob fail"+e.getMessage());
		}finally{
			if (lob != null){
				lob.close();
			}
		}
		return oid;
				
	}
	
	void readLob(DBCollection cl,ObjectId oid){
		DBLob rLob = null;
		try
		{
			rLob = cl.openLob(oid);
			byte[] rbuff = new byte[1024];
			int readLen =0;		
			ByteBuffer bytebuff = ByteBuffer.allocate((int)rLob.getSize());
			while ((readLen = rLob.read(rbuff)) != -1){
				bytebuff.put(rbuff, 0, readLen);				
			}
			bytebuff.rewind();				
			
			String curMd5 = LobUtils.getMd5(bytebuff);
			String prevMd5 = id2md5.get(oid);
			Assert.assertEquals(curMd5, prevMd5);
			id2md5.remove(oid);
		}catch(BaseException e){
			Assert.assertTrue(false,"read lob fail"+e.getMessage());
		}finally{
			if (null != rLob)
				rLob.close();
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
		}
	}	
	
		
	
	@Test(invocationCount = 5, threadPoolSize = 5)
	public void testPutAndReadLob(){
		Sequoiadb db  = null;
		try{
			db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			BSONObject obj = (BSONObject) JSON.parse("{PreferedInstance:\"M\"}");
			db.setSessionAttr(obj);
			CollectionSpace cs = db.getCollectionSpace(SdbTestBase.csName);
			DBCollection cl = cs.getCollection(clName);
			ObjectId id = putLob(cl);
			readLob(cl, id);
		}catch(BaseException e){
		   e.printStackTrace();
		   Assert.assertTrue(false, e.getMessage()+e.getStackTrace());	
		}finally{
			if(db != null){
				db.disconnect();   
			}
		}
	}	
	
}


