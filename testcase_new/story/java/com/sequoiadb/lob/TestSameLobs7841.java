package com.sequoiadb.lob;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.BeforeClass;

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
	private Random random = new Random();	
	private byte[] wlobBuff = null;   
	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+coordUrl+e.getMessage());
		}
		createCL();
		int writeLobSize = random.nextInt(1024*1024);	
		wlobBuff = LobOprUtils.getRandomBytes(writeLobSize);
	}		
	
	//write same lob 
	@Test(invocationCount = 100,threadPoolSize = 100)
	private void testSameLob(){	
		Sequoiadb db = null;
		try{
			db = new Sequoiadb(SdbTestBase.coordUrl, "", "") ;
			DBCollection dbcl = db.getCollectionSpace(SdbTestBase.csName).getCollection(clName);
			
			//write lob
			ObjectId oid = LobOprUtils.createAndWriteLob(dbcl, wlobBuff);
			
			//read lob and check the lobdata
			byte[] rbuff = new byte[ wlobBuff.length];
			DBLob rLob= dbcl.openLob(oid);		
			rLob.read(rbuff);			
			rLob.close();			
			LobOprUtils.assertByteArrayEqual(rbuff, wlobBuff, "lob data is wrong!the oid: " + oid.toString());
		}finally{
			if ( db != null ){
				db.disconnect();
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
			if( null != sdb){
				sdb.disconnect();
			}
		}
	}	
	
	private void createCL(){						
	    try
	    {
	    	String clOptions = "{ReplSize:0}";
	    	BSONObject options =(BSONObject) JSON.parse(clOptions);	    	
		    cs = sdb.getCollectionSpace(SdbTestBase.csName);			
		    cs.createCollection(clName,options);			
	    }catch(BaseException e){
		    Assert.assertTrue(false,"create cl fail "+e.getErrorType()+":"+e.getMessage());
	    }
	 }	
	
	
	
		
	
	
	
	
	
	
}

