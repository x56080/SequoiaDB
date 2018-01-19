package com.sequoiadb.lob;

import java.util.Arrays;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

import org.bson.types.ObjectId;
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
import com.sequoiadb.testcommon.SdbThreadBase;

/**
* FileName: TestConcurrentWriteLob10423.java
* test content:Concurrent write lob ,contains the same oid and different oid
* testlink case:seqDB-10423
* @author wuyan
    * @Date    2016.9.12
* @version 1.00
*/
public class TestConcurrentWriteLob10423 extends SdbTestBase {
	private String clName = "cl_lob10423";	
	private static Sequoiadb sdb = null;
	private CollectionSpace cs = null;	
	private Random random = new Random();
	private AtomicInteger sameOidWriteOKCount = new AtomicInteger(0);
    	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+SdbTestBase.coordUrl+e.getMessage());
		}
		createCL();
	}		
		
	@Test
	public void testLob(){		
		WriteAndReadLobTask writeDiffLobTask = new WriteAndReadLobTask();
		writeDiffLobTask.start(50);
		
		WriteSameOidLobTask writeSameLobTask = new WriteSameOidLobTask();
		writeSameLobTask.start(5);
		
		Assert.assertTrue( writeDiffLobTask.isSuccess(), writeDiffLobTask.getErrorMsg());
		Assert.assertTrue( writeSameLobTask.isSuccess(), writeSameLobTask.getErrorMsg());
		//write lob of same oid only one success
		int expSuccessNum = 1;
		Assert.assertEquals(sameOidWriteOKCount.get(), expSuccessNum);	
	}

	@AfterClass
	public void tearDown(){		
		try{
			if(cs.isCollectionExist(clName)){
				cs.dropCollection(clName);
			}
		}catch(BaseException e){			
			Assert.assertTrue(false,"clean up failed:"+e.getMessage());
		}finally {
			if( sdb != null){
				sdb.disconnect();
			}
		}
	}	
	
	private class WriteAndReadLobTask extends SdbThreadBase {		
		@Override
		public void exec() throws Exception {			
			int writeLobSize = random.nextInt(1024*1024);;
			byte[] lobBuff = LobOprUtils.getRandomBytes(writeLobSize);	
			Sequoiadb sdb = null;
		    try{	
		    	sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		    	DBCollection cl = sdb.getCollectionSpace(SdbTestBase.csName).getCollection(clName);	
		    	ObjectId oid = null;				
				DBLob lob = cl.createLob();
				lob.write(lobBuff);	
				oid = lob.getID();	
				lob.close();				
				
				//read and check the lob data
				DBLob rLob = cl.openLob(oid);
				byte[] rbuff = new byte[(int) rLob.getSize()];
				rLob.read(rbuff);
				rLob.close();
				Arrays.equals(rbuff, lobBuff);
				rLob.close();
			}finally{
		    	if ( sdb != null ){
		    		sdb.disconnect();
		    	}
		    }
		}
	}
	
	private class WriteSameOidLobTask extends SdbThreadBase {		
		@Override
		public void exec() throws Exception {			
			int writeLobSize = random.nextInt(1024*1024);;
			byte[] lobBuff = LobOprUtils.getRandomBytes(writeLobSize);	
			ObjectId oid = new ObjectId("5a3b6f23c5d07c3000f73a8b");
			
			Sequoiadb sdb = null;
		    try{	
		    	sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		    	DBCollection cl = sdb.getCollectionSpace(SdbTestBase.csName).getCollection(clName);			    	
				DBLob lob = cl.createLob(oid);
				lob.write(lobBuff);	
				oid = lob.getID();	
				lob.close();
				
				//read and check the lob data
				DBLob rLob = cl.openLob(oid);
				byte[] rbuff = new byte[(int) rLob.getSize()];
				rLob.read(rbuff);	
				rLob.close();
				Arrays.equals(rbuff, lobBuff);

				//recorded the numbers of write lob successful 
				sameOidWriteOKCount.getAndIncrement();
		    }catch(BaseException e){		    	
		    	if ( e.getErrorCode() != -297 && e.getErrorCode() != -269&& e.getErrorCode() != -5){
		    		e.getStackTrace();
		    		Assert.assertTrue(false,"same oid write fail "+e.getErrorType()+":"+e.getMessage());
		    	}			    
		    }finally {
				if ( sdb != null){
					sdb.disconnect();
				}
			}
		}
	}
	
	public void createCL(){						
	    try
	    {
	    	 cs = sdb.getCollectionSpace(SdbTestBase.csName);			
		     cs.createCollection(clName);			
	    }catch(BaseException e){
		    Assert.assertTrue(false,"create cl fail "+e.getErrorType()+":"+e.getMessage());
	    }
	 }		
}


