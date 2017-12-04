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
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadataconsistency.data.MetaDataUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
* TestLink: seqDB-10212: concurrency[createIndex, dropCL]
* @author xiaoni huang init
* @Date   2016.10.17
*/

public class Index10212 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private static Sequoiadb sdb = null;
	private String csName = "cs10212";
	private String clName = "cl10212";
	private String idxName = "idx";
	
	@BeforeClass
	public void setUp(){
		//start time
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			//judge the mode
			if(MetaDataUtils.isStandAlone(sdb)){
				throw new SkipException("The mode is standlone, skip the testCase.");
			}
			MetaDataUtils.clearCS(sdb, csName);
			
			CollectionSpace csDB = sdb.createCollectionSpace(csName);
			csDB.createCollection(clName);
			MetaDataUtils.insertData(sdb, csName, clName);
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
	
	@Test
	public void test(){
		CreateIndex createIndex = new CreateIndex();
		createIndex.start();

		DropCL dropCL = new DropCL();
		dropCL.start();
		
		if( !( createIndex.isSuccess() && dropCL.isSuccess() ) ){
			Assert.fail(createIndex.getErrorMsg() + dropCL.getErrorMsg());
		}

		//check results
		MetaDataUtils.checkIndex(csName, clName);
	}

	private class CreateIndex extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				CollectionSpace csDB = db.getCollectionSpace(csName);
				
				String name = idxName;
				Random i = new Random();
				BSONObject opt = new BasicBSONObject();
				opt.put("a" + i.nextInt(10000), 1);
				DBCollection clDB = csDB.getCollection(clName);
				if(clDB != null){
					clDB.createIndex(name + i.nextInt(100), opt, false, false);
				}
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -247  //-247:Redefine index
						&& eCode != -46  //-46:Duplicate index name
						&& eCode != -23 
						&& eCode != -34  //-34:because is only one CL in CS, delete the CS data file when deleting the last CL, so exception -34 when creatIndex 
						&& eCode != -108){ 
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}

	private class DropCL extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				db.getCollectionSpace(csName).dropCollection(clName);
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -147 ){ 
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}
	
}