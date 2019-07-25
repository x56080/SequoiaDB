package com.sequoiadb.lob;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;

import java.util.Arrays;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;


/**
* FileName: TestSeekLob7839.java
* test content:lob seek 
* testlink case:seqDB-7839
* @author wuyan
    * @Date    2016.9.12    * 
* @version 1.00
* update:  wuyan  2017.12.19
*/

public class TestSeekLob7839 extends SdbTestBase {
	@DataProvider(name = "pagesizeProvider",parallel = true)
	public Object[][] generatePageSize(){
		return new Object[][]{
			//the parameter : offset and readsize
			//test a: seek read in one group
			new Object[]{1024, 1024*1024},
			//test c: seek read in one piece
			new Object[]{1024*255, 1024*510},
			//test d: seek read in mulitple pieces
			new Object[]{1024*2, 1024*1024*1},		
		};
	}

	private String clName = "cl_lob7839";
	private Sequoiadb sdb = null;
	private CollectionSpace cs = null;
	private DBCollection cl = null;
	private ObjectId oid = null;
	private byte[] wlobBuff = null;
    
	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+coordUrl+e.getMessage());
		}
		
		createCL( );
		//write lob
		int writeLobSize = 1024*1024*2;
		wlobBuff = LobOprUtils.getRandomBytes(writeLobSize);
		oid = LobOprUtils.createAndWriteLob(cl, wlobBuff);		
	}
	
	@Test(dataProvider = "pagesizeProvider")
	public void testSeekAndReadLob( int offset, int readsize){
		Sequoiadb db = null;
		try{
			db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			db.setSessionAttr(new BasicBSONObject("PreferedInstance", "M"));
			DBCollection dbcl = db.getCollectionSpace(SdbTestBase.csName).getCollection(clName);
			DBLob rLob = dbcl.openLob( oid);
			byte[] rbuff = new byte[readsize];
			rLob.seek(offset, DBLob.SDB_LOB_SEEK_SET);
			rLob.read(rbuff);
			rLob.close();
			byte[] expBuff = Arrays.copyOfRange(wlobBuff, offset, offset+readsize);			
			LobOprUtils.assertByteArrayEqual(rbuff, expBuff, "lob data is wrong!the oid: " + oid.toString());
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
			if( null != sdb ){
				sdb.disconnect();				
			}
		}	
	}
	
	private void createCL(){						
	    try
	    {
		    cs = sdb.getCollectionSpace(SdbTestBase.csName);			
		    cl = cs.createCollection(clName);			
	    }catch(BaseException e){
		    Assert.assertTrue(false,"create cl fail "+e.getErrorType()+":"+e.getMessage());
	    }
	 }	
	
	

	
}


