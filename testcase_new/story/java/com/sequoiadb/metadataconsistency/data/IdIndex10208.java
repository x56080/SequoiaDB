package com.sequoiadb.metadataconsistency.data;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.AfterClass;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadataconsistency.data.MetaDataUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
* TestLink: seqDB-10208: concurrency[dropIndex in mainCL]
* @author xiaoni huang init
* @Date   2016.10.20
*/

public class IdIndex10208 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private static Sequoiadb sdb = null;
	private String csName = "cs10208";
	private String clName = "cs10208";
	private String mCLName = clName + "_m";
	private String sCLName = clName + "_s";
	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			//judge the mode and group number
			if(MetaDataUtils.isStandAlone(sdb)){
				throw new SkipException("The mode is standlone, skip the testCase.");
			}
			MetaDataUtils.clearCS(sdb, csName);
			
			sdb.createCollectionSpace(csName);
			createMainCL(sdb);
			createSubCL(sdb);
			attachCL(sdb);
			createIdIndex(sdb);
			MetaDataUtils.insertData(sdb, csName, mCLName);
		}catch(BaseException e){
			sdb.disconnect();
			Assert.fail(e.getMessage());
		}
	}
	
	@AfterClass
	public void tearDown(){
		try{
			MetaDataUtils.clearCS(sdb, csName);
		}catch(BaseException e){
			Assert.fail(e.getMessage());
		}finally{
			sdb.disconnect();
		}
	}
	
	@Test(invocationCount = 10, threadPoolSize = 10)
	public void test(){
		DropIndex dropIndex = new DropIndex();
		dropIndex.start();
		
		if( !dropIndex.isSuccess() ){
			Assert.fail(dropIndex.getErrorMsg());
		}

		//check results
		MetaDataUtils.checkIndex(csName, clName);
	}

	private class DropIndex extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				DBCollection clDB = db.getCollectionSpace(csName).getCollection(mCLName);
				clDB.dropIdIndex();
			}catch(BaseException e){
				if(e.getErrorCode() != -247 && e.getErrorCode() != -147){
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}
	
	public void createMainCL(Sequoiadb sdb){
		try{
			BSONObject opt = new BasicBSONObject();
			BSONObject subObj = new BasicBSONObject();
			subObj.put("a", 1);
			opt.put("ShardingKey", subObj); 
			opt.put("ReplSize", 0);
			opt.put("IsMainCL", true);
			sdb.getCollectionSpace(csName).createCollection(mCLName, opt);
		}catch(BaseException e){
			throw e;
		}
	}
	
	public void createSubCL(Sequoiadb sdb){
		try{
			BSONObject opt = new BasicBSONObject();
			BSONObject subObj = new BasicBSONObject();
			subObj.put("a", 1);
			opt.put("ShardingKey", subObj);
			opt.put("ReplSize", 0);
			for(int i = 0; i < 3; i++){
				sdb.getCollectionSpace(csName).createCollection(sCLName + i, opt);
			}
		}catch(BaseException e){
			throw e;
		}
	}
	
	public void attachCL(Sequoiadb sdb){
		try
		{
			DBCollection clDB = sdb.getCollectionSpace(csName).getCollection(mCLName);
					
			BSONObject options = new BasicBSONObject();
			BSONObject lowBoundObj = new BasicBSONObject();
			BSONObject upBoundObj  = new BasicBSONObject();
			for(int i = 0; i < 3; i++){
				int bound = i * 100;
				lowBoundObj.put("a", bound);
				upBoundObj.put("a", bound + 100);
				options.put("LowBound", lowBoundObj);
				options.put("UpBound", upBoundObj);
				clDB.attachCollection(csName + "." + sCLName + i, options);
			}
		}catch(BaseException e){
			throw e;
		}
	}
	
	public void createIdIndex(Sequoiadb sdb){
		try{
			DBCollection clDB = sdb.getCollectionSpace(csName).getCollection(mCLName);
			BSONObject opt2 = new BasicBSONObject();
			opt2.put("SortBufferSize", 128);
			clDB.createIdIndex(opt2);
		}catch(BaseException e){
			throw e;
		}
	}	
}