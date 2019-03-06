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
* TestLink: 
* 		seqDB-10171: concurrency[createCL, alterCL, dropCL]
* @author xiaoni huang init
* @Date   2016.10.11
*/

public class CL10171 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private static Sequoiadb sdb = null;
	private String csName = "cs10171";
	private String clName = "cl10171";
	private Random random = new Random();
	private int number = 20;
	private int msec = 100;
	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			//judge the mode or node number
			if(MetaDataUtils.isStandAlone(sdb) || MetaDataUtils.oneDataNode(sdb)){
				throw new SkipException("The mode is standlone or one node, skip the testCase.");
			}
			MetaDataUtils.clearCS(sdb, csName);
			sdb.createCollectionSpace(csName);
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

		CreateCL createCL = new CreateCL();
		createCL.start();

		AlterCL alterCL = new AlterCL();
		MetaDataUtils.sleep(random.nextInt(msec));
		alterCL.start();

		DropCL dropCL = new DropCL();
		MetaDataUtils.sleep(random.nextInt(msec));
		dropCL.start();
		
		if( !( createCL.isSuccess() && alterCL.isSuccess() && dropCL.isSuccess() ) ){
			Assert.fail(createCL.getErrorMsg() + alterCL.getErrorMsg() 
					+ dropCL.getErrorMsg());
		}
		
		//check results
		MetaDataUtils.checkCLResult(csName, clName);
	}
	
	private class CreateCL extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				CollectionSpace csDB = db.getCollectionSpace(csName);
				
				String tmpCLName = clName + "_" + random.nextInt(number);
				csDB.createCollection(tmpCLName);
				if(csDB.isCollectionExist(tmpCLName)){
					MetaDataUtils.insertData(db, csName, tmpCLName);
				}
				
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -22 && eCode != -147){   
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}	
	}
	
	private class AlterCL extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				CollectionSpace csDB = db.getCollectionSpace(csName);
				
				BSONObject opt = new BasicBSONObject();
				opt.put("ReplSize", 1 );
				if(csDB.isCollectionExist(clName)){
					csDB.getCollection(clName + "_" + random.nextInt(number)).alterCollection(opt);
				}
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -23 && eCode != -147 ){  
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
			Sequoiadb db = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				CollectionSpace csDB = db.getCollectionSpace(csName);
				
				csDB.dropCollection(clName +"_"+ random.nextInt(number));
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -23 && eCode != -147 ){  
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}
			
}
