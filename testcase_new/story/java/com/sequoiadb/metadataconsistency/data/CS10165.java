package com.sequoiadb.metadataconsistency.data;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.AfterClass;
import org.testng.Assert;
import org.testng.SkipException;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadataconsistency.data.MetaDataUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
* TestLink: 
* 		seqDB-10165: concurrency[createCS, dropCS]
* @author xiaoni huang init
* @Date   2016.9.25
*/

public class CS10165 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private static Sequoiadb sdb = null;
	private String csName = "cs10165";
	private Random random = new Random();
	private int number = 30;
	private int msec = 100;
	
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
		CreateCS createCS = new CreateCS();
		createCS.start();

		DropCS dropCS = new DropCS();
		MetaDataUtils.sleep(random.nextInt(msec));
		dropCS.start();
		
		if( !( createCS.isSuccess() && dropCS.isSuccess() ) ){
			Assert.fail(createCS.getErrorMsg() + dropCS.getErrorMsg());
		}

		//check results
		MetaDataUtils.checkCSOfCatalog(csName);
	}

	private class CreateCS extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			    db.createCollectionSpace(csName + "_" + random.nextInt(number));
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -33 && eCode != -147){ 
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}

	private class DropCS extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				db.dropCollectionSpace(csName + "_" + random.nextInt(number));
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