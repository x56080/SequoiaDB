package com.sequoiadb.metadataconsistency.data;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.AfterClass;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadataconsistency.data.MetaDataUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
* TestLink: seqDB-10186: concurrency[attachCL]
* @author xiaoni huang init
* @Date   2016.10.11
*/

public class SubCL10186 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private static Sequoiadb sdb = null;
	private String csName = "cs10186";
	private String clName = "cl10186";
	private String mCLName = clName + "_m";
	private String sCLName = clName + "_s";
	private Random random = new Random();
	private int number = 20;
	
	@BeforeClass
	public void setUp(){
		//start time
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			//judge the mode or node number
			if(MetaDataUtils.isStandAlone(sdb) || MetaDataUtils.oneCataNode(sdb) 
					|| MetaDataUtils.oneDataNode(sdb)){
				throw new SkipException("The mode is standlone or one node, skip the testCase.");
			}
			MetaDataUtils.clearCS(sdb, csName);
			
			sdb.createCollectionSpace(csName);
			createMainCL(sdb);
			createSubCL(sdb);
		}catch(BaseException e){
			sdb.disconnect();
			Assert.fail(e.getMessage());
		}
	}
	
	@AfterClass
	public void tearDown(){
		try{
			MetaDataUtils.clearCL(sdb, csName, clName);
			MetaDataUtils.clearCS(sdb, csName);
		}catch(BaseException e){
			Assert.fail(e.getMessage());
		}finally{
			sdb.disconnect();
		}
	}
	
	@Test(invocationCount = 3, threadPoolSize = 3)
	public void test(){

		AttachCL attachCL = new AttachCL();
		attachCL.start();
		
		if( !attachCL.isSuccess() ){
			Assert.fail(attachCL.getErrorMsg());
		}
		
		//check results
		MetaDataUtils.checkCLResult(csName, clName);
	}
	
	private class AttachCL extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				CollectionSpace csDB = db.getCollectionSpace(csName);
					
				BSONObject options = new BasicBSONObject();
				BSONObject lowBoundObj = new BasicBSONObject();
				BSONObject upBoundObj  = new BasicBSONObject();
				int k = random.nextInt(10000);
				lowBoundObj.put("a", 0 + k);
				upBoundObj.put("a", 100 + k);
				options.put("LowBound", lowBoundObj);
				options.put("UpBound", upBoundObj);
				csDB.getCollection(mCLName + random.nextInt(number)).
						attachCollection(csName + "." + sCLName + random.nextInt(number), options);
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -235 //-235:Duplicated attach collection partition
						&& eCode != -237){  //-237:New boundary is conflict with the existing boundary
					throw e;
				}
			}finally{
				db.disconnect();
			}
			
		}
	}
	
	public void createMainCL(Sequoiadb sdb){
		try{
			CollectionSpace csDB = sdb.getCollectionSpace(csName);
			
			BSONObject mOpt = new BasicBSONObject();
			BSONObject mSubObj = new BasicBSONObject();
			mSubObj.put("a", 1);
			mOpt.put("ShardingKey", mSubObj); 
			mOpt.put("ReplSize", 0);
			mOpt.put("IsMainCL", true);
			for(int i = 0; i < number; i++){
				csDB.createCollection( mCLName + i, mOpt );
			}
		}catch(BaseException e){
			throw e;
		}
	}
	
	public void createSubCL(Sequoiadb sdb){
		try{
			CollectionSpace csDB = sdb.getCollectionSpace(csName);
			
			BSONObject sOpt = new BasicBSONObject();
			BSONObject sSubObj = new BasicBSONObject();
			sSubObj.put("a", 1);
			sOpt.put("ShardingKey", sSubObj);
			sOpt.put("ReplSize", 0);
			for(int i = 0; i < number; i++){
				csDB.createCollection( sCLName + i, sOpt );
			}
		}catch(BaseException e){
			throw e;
		}
	}
		
}