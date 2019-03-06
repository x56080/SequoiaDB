package com.sequoiadb.metadataconsistency.data;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.AfterClass;
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
* TestLink: 
* 		seqDB-10173: concurrency[createCL, dropCS]
* @author xiaoni huang init
* @Date   2016.10.11
*/

public class CL10173 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private static Sequoiadb sdb = null;
	private String csName = "cs10173";
	private String clName = "cl10173";
	private Random random = new Random();
	private int msec = 100;
	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			//judge the mode or group number or node number
			if(MetaDataUtils.isStandAlone(sdb) || MetaDataUtils.OneGroupMode(sdb)
					|| MetaDataUtils.oneCataNode(sdb) || MetaDataUtils.oneDataNode(sdb)){
				throw new SkipException("The mode is standlone or only one group or one node, "
						+ "skip the testCase.");
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

		DropCS dropCS = new DropCS();
		MetaDataUtils.sleep(random.nextInt(msec));
		dropCS.start();
		
		if( !( createCL.isSuccess() && dropCS.isSuccess() ) ){
			Assert.fail(createCL.getErrorMsg() + dropCS.getErrorMsg());
		}
		
		//check results
		MetaDataUtils.checkCLResult(csName, clName);
	}

	private class CreateCL extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				CollectionSpace csDB = db.getCollectionSpace(csName);
				
				if(csDB != null ){
					DBCollection clDB = csDB.createCollection(clName);
					if(clDB != null){
						MetaDataUtils.insertData(db, csName, clName);
					}
				}
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -23 && eCode != -34 
						&& eCode != -248 && eCode != -147 ){ 
					throw e;
				}
			}
			finally{
				db.disconnect();
			}
		}
	}

	private class DropCS extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				db.dropCollectionSpace(csName);
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -34 && eCode != -147){  
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}
}