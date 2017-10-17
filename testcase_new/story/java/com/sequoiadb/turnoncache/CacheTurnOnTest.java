package com.sequoiadb.turnoncache;

import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.ClientOptions;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

import com.sequoiadb.testcommon.*;
public class CacheTurnOnTest extends SdbTestBase{
	private Sequoiadb db = null;
	private Sequoiadb db_check = null;
	private Random r = new Random();
	private String testCaseCSName;
	private String clName;
	private String expectRes;
	
	
	@DataProvider(name = "clientoption-provider")
	public Object[][] getClientOption(){
		return new Object[][]{
				new Object[]{true, 20000},
				new Object[]{false, 1000},
		};
	}
	
	@BeforeClass
	void init(){
		try{
			db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			db_check = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		}catch(BaseException e){
			Assert.fail("connect " + SdbTestBase.coordUrl + " failed." + e.getMessage());
		}
		
		
		if (testCaseCSName != null){
			testCaseCSName = csName + "test_cache";
		}else{
			testCaseCSName = "java_test_cache";
		}
		clName = "java_test_cache";
		testCaseCSName += r.nextInt();
		clName += r.nextInt();
		
		expectRes = "SDB_DMS_NOTEXIST";
		if (isStandAlone()){
			expectRes = "SDB_DMS_CS_NOTEXIST";
		}
	}
	
	@AfterMethod
	void clearCS(){
		try{
			if(db.isCollectionSpaceExist(testCaseCSName)){
				db.dropCollectionSpace(testCaseCSName);
			}
		}catch(BaseException e){
			Assert.assertFalse(true, e.getMessage());
		}
	}
	
	void initClient(boolean enable, int inteval){
		ClientOptions option = new ClientOptions();
		option.setCacheInterval(inteval);
		option.setEnableCache(enable);
		
		Assert.assertEquals(option.getEnableCache(), enable);
		Assert.assertEquals(option.getCacheInterval(), inteval);
		Sequoiadb.initClient(option);
	}
	
	long getCS(Sequoiadb db) throws BaseException{
		long begin = System.nanoTime();
		try{
			db.getCollectionSpace(testCaseCSName);
		}catch(BaseException e){
			throw e;
		}
		long end = System.nanoTime();
		return end - begin;
	}
	
	long getCL(Sequoiadb db)throws BaseException{
		long begin = System.nanoTime();
		try{
			CollectionSpace cs = db.getCollectionSpace(testCaseCSName);
			DBCollection cl = cs.getCollection(clName);
			if (cl == null){
				throw new BaseException("SDB_DMS_NOTEXIST");
			}
		}catch(BaseException e){
			throw e;
		}
		
		long end = System.nanoTime();
		return end - begin;
	}
	
	
	CollectionSpace createCS(){
		CollectionSpace cs = null;
		try{
			cs = db.createCollectionSpace(testCaseCSName);
		}catch(BaseException e){
			Assert.assertFalse(true,e.getMessage());
		}
		return cs;
	}
	
	CollectionSpace createCL(BSONObject options){
		CollectionSpace cs = null;
		try{
			cs = db.createCollectionSpace(testCaseCSName);
			if (options != null){
				cs.createCollection(clName, options);
			}else{
				cs.createCollection(clName);
			}
		}catch(BaseException e){
			if(e.getErrorCode() != 
					new BaseException("SDB_DMS_CS_NOTEXIST").getErrorCode()){
				Assert.assertFalse(true, e.getMessage());
			}
		}
		return cs;
	}
	
	@SuppressWarnings("deprecation")
	void dropCS(CollectionSpace cs){
		try{
			int selector = r.nextInt(2);
			if (selector == 1){
			   db.dropCollectionSpace(testCaseCSName);
			}else{
				cs.drop();
			}
		}catch(BaseException e){
			Assert.assertFalse(true, e.getMessage());
		}
	}
	
	void dropCL(CollectionSpace cs){
		try{
			cs.dropCollection(clName);
		}catch(BaseException e){
			Assert.assertFalse(true, e.getMessage());
		}	
	}
	
	@Test(dataProvider= "clientoption-provider")
	void testCreateCS(boolean enable, int inteval){
		initClient(enable, inteval);
		CollectionSpace  cs = createCS();
		
		try{
			getCS(db_check);
			dropCS(cs);
			getCS(db_check);
		}catch(BaseException e){
			if (enable){
				Assert.assertFalse(true,e.getMessage());
			}else{
				Assert.assertEquals(e.getErrorCode(), 
						new BaseException("SDB_DMS_CS_NOTEXIST").getErrorCode(), e.getMessage());
			}
		}
	}
	
	@Test(dataProvider= "clientoption-provider")
	void testCreateCL(boolean enable, int inteval){
		initClient(enable, inteval);
		CollectionSpace cs = createCL(null);
		try{
			getCL(db_check);
			dropCL(cs);
			getCL(db_check);
			
		}catch(BaseException e){
			if (enable){
				Assert.assertFalse(true,e.getMessage());
			}else{
				Assert.assertEquals(e.getErrorCode(), 
						new BaseException("SDB_DMS_NOTEXIST").getErrorCode(), e.getMessage());
			}
		}
	}
	
	@Test(dataProvider= "clientoption-provider")
	void testCreateCLWithOptions(boolean enable, int inteval){
		initClient(enable, inteval);
		BSONObject options = new BasicBSONObject();
		options.put("ReplSize", 0);
		CollectionSpace cs = createCL(options);
		
		try{
			getCL(db_check);
			dropCL(cs);
			getCL(db_check);
		}catch(BaseException e){
			if (enable){
				Assert.assertFalse(true,e.getMessage());	
			}else{
				Assert.assertEquals(e.getErrorCode(), 
						new BaseException("SDB_DMS_NOTEXIST").getErrorCode(), e.getMessage());
			}
		}
	}
	
	@Test(dataProvider= "clientoption-provider")
	void testDropCS(boolean enable, int inteval){
		initClient(enable, inteval);
		CollectionSpace cs = createCS();
		dropCS(cs);
		try{
			getCS(db);
			Assert.assertFalse(true, "must is SDB_DMS_CS_NOTEXIST");
		}catch(BaseException e){
			Assert.assertEquals(e.getErrorCode(), 
				new BaseException("SDB_DMS_CS_NOTEXIST").getErrorCode());
		}
	}
	
	@Test(dataProvider= "clientoption-provider")
	void testDropCL(boolean enable, int inteval){
		initClient(enable, inteval);
		CollectionSpace cs = createCL(null);
		dropCL(cs);
		
		try{
			getCL(db);
			Assert.assertFalse(true, "must is SDB_DMS_NOTEXIST");
		}catch(BaseException e){
			Assert.assertEquals(e.getErrorCode(), 
			      new BaseException("SDB_DMS_NOTEXIST").getErrorCode());
		}
		dropCS(cs);
	}
	
	@Test(dataProvider= "clientoption-provider")
	void testGetCSOfTimeOut(boolean enable, int inteval){
		initClient(enable, inteval);
		CollectionSpace cs = createCS();
		
		try{
			getCS(db_check);
			Thread.sleep(inteval+2000);
			dropCS(cs);
			getCS(db_check);
			Assert.assertFalse(true, "must is SDB_DMS_CS_NOTEXIST");
		}catch(BaseException e){
			Assert.assertEquals(e.getErrorCode(), 
					new BaseException("SDB_DMS_CS_NOTEXIST").getErrorCode());
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			Assert.assertFalse(true, e.getMessage());
		}
	}
	
	@Test(dataProvider= "clientoption-provider")
	void testGetCLOfTimeOut(boolean enable, int inteval){
		initClient(enable, inteval);
		CollectionSpace cs = createCL(null);
		
		try{
			getCL(db_check);
			Thread.sleep(inteval+2000);
			dropCL(cs);
			getCL(db_check);
			Assert.assertFalse(true, "must is SDB_DMS_CS_NOTEXIST");
		}catch(BaseException e){
			Assert.assertEquals(e.getErrorCode(), 
				      new BaseException("SDB_DMS_NOTEXIST").getErrorCode());
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			Assert.assertFalse(true, e.getMessage());
		}
		dropCS(cs);
	}
	
	boolean isStandAlone(){
		try{
			db.getReplicaGroupNames();
		}catch(BaseException e){
			if (e.getErrorCode() == new BaseException("SDB_RTN_COORD_ONLY").getErrorCode()){
				return true;
			}
		}
		return false;
	}
	
	@Test(dataProvider= "clientoption-provider")
	void testGetCLAfterDropCS(boolean enable, int inteval){
		initClient(enable, inteval);
		CollectionSpace cs = createCL(null);
		dropCS(cs);
		
		try{
			cs.getCollection(clName);
		}catch(BaseException e){
			Assert.assertEquals(e.getErrorCode(), new BaseException(expectRes).getErrorCode());
		}
	}
	
	@Test(dataProvider= "clientoption-provider")
	void testUpdateTimeStamp(boolean enable, int inteval){
		initClient(enable, inteval);
		CollectionSpace cs = createCL(null);
	
		try {
			Thread.sleep(inteval);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			Assert.fail(e.getMessage());
		}
		try{
			DBCollection cl = cs.getCollection(clName);
			BSONObject doc = new BasicBSONObject();
			doc.put("_id", r.nextInt());
			cl.insert(doc);
		}catch(BaseException e){
			Assert.assertFalse(true, e.getMessage());
		}
		
		try{
			db_check.dropCollectionSpace(testCaseCSName);
			getCS(db);
		}catch(BaseException e){
			if (enable){
			   Assert.assertFalse(true, e.getMessage());
			}else{
				Assert.assertEquals(e.getErrorCode(), new BaseException("SDB_DMS_CS_NOTEXIST").getErrorCode());	
			}
		}
		
		String expectRes = "SDB_DMS_CS_NOTEXIST";
		if (isStandAlone()){
			expectRes = "SDB_DMS_NOTEXIST";
		}
		try{
			getCL(db);
		}catch(BaseException e){
			if (enable){
				Assert.assertFalse(true, e.getMessage());	
			}else{
				Assert.assertEquals(e.getErrorCode(), new BaseException("SDB_DMS_CS_NOTEXIST").getErrorCode());	
			}
		}
	}
	
	@AfterClass
	void fini(){
		try{
			if (db != null){
				db.disconnect();
			}
			
			if (db_check != null){
				db_check.disconnect();
			}
		}catch(BaseException e){
			// 结束时忽略
		}
	}
}
