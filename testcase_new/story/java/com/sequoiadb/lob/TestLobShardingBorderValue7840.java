package com.sequoiadb.lob;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;

import java.nio.ByteBuffer;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
* FileName: TestLobShardingBorderValue7840.java
* test content:test boundary value of the lob sharding.
* testlink case:seqDB-7840
* @author wuyan
    * @Date    2016.9.12
* @version 1.00
*/
public class TestLobShardingBorderValue7840 extends SdbTestBase {
	@DataProvider(name = "pagesizeProvider")
	public Object[][] generatePageSize(){
		return new Object[][]{		
				
			//sharding after just over 1 pages,lobsize is 768k
			new Object[]{0,786432},
		    //sharding after less than 1kb, lobsize is 511k
			new Object[]{0,523264},
	        //sharding after more than 1kb,lobsize is 266k
			new Object[]{0, 263168},
			//sharding after just over 1 pages,lobsize is 512k
			new Object[]{524288, 524288},
			//sharding after less than 1kb, lobsize is 511k
			new Object[]{524288, 523264},
			//sharding after more than 1kb,lobsize is 513k
			new Object[]{524288, 525312},
		};
	}
	
	private String csName = "cs_lob7840";
	private String clName = "cl_lob7840";
	private Sequoiadb sdb = null;
	private CollectionSpace cs = null;
	private DBCollection cl = null;    
	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+coordUrl+e.getMessage());
		}		
	}
		
	private void createCL(int lobPagesize){
		if (sdb.isCollectionSpaceExist(csName)){
			sdb.dropCollectionSpace(csName);
		}
		
		BSONObject options = new BasicBSONObject();
		options.put("LobPageSize", lobPagesize);		
		try
		{
			cs = sdb.createCollectionSpace(csName, options);	
			cl = cs.createCollection(clName);
		}catch(BaseException e){
			Assert.assertTrue(false,"create CS/CL fail "+e.getErrorType()+":"+e.getMessage());			
		}
	}
	
	private void dropCS(){
		try{
			sdb.dropCollectionSpace(csName);		
		}catch(BaseException e){
			Assert.assertTrue(false,"create CS/CL fail "+e.getErrorType()+":"+e.getMessage());
		}
	}	
	
	/**
	 * put and read lob ,then check write and read stream MD5 value
	 * @param length
	 *        write lob size
	 */	
	private void putLob(int length){
		String lobSb = LobOprUtils.getRandomString(length);
		ObjectId oid  = null;			
		String prevMd5 = "";
		DBLob lob = null;
		try{			
			lob = cl.createLob();
			lob.write(lobSb.getBytes());
		
			prevMd5 = LobOprUtils.getMd5(lobSb);
		    oid = lob.getID();
		}finally{
			if (lob != null){
				lob.close();
			}
		}
				
		DBLob rLob = null;
		try
		{
			rLob = cl.openLob(oid);
			
			byte[] rbuff = new byte[1024];			
			int readLen =0;			
			ByteBuffer bytebuff = ByteBuffer.allocate((int)length);			
			while ((readLen = rLob.read(rbuff)) != -1){			
				bytebuff.put(rbuff, 0, readLen);				
			}			
			bytebuff.rewind();
		
			String curMd5 = LobOprUtils.getMd5(bytebuff);		
			Assert.assertEquals(prevMd5, curMd5);
		}finally{
			if (rLob != null){
				rLob.close();
			}
		}
	}
	
	@AfterClass
	public void tearDown(){		
		try{			
			sdb.disconnect();
		}catch(BaseException e){			
			Assert.assertTrue(false,"clean up failed:"+e.getMessage());
		}finally{
			if( sdb != null ){
				sdb.disconnect();
			}
		}
	}
	
	
	@Test(dataProvider = "pagesizeProvider")
	public void putLobinAnyPageSize(int lobPageSize, int length){
		createCL(lobPageSize);			
		putLob(length);
		dropCS();		
	}		
}


