package com.sequoiadb.metadataconsistency.cluster;

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
import com.sequoiadb.metadataconsistency.data.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
* TestLink: seqDB-10225、seqDB-10226: concurrency[createRG, removeRG]
* @author xiaoni huang init
* @Date   2016.10.24
*/

public class Group10225 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private static Sequoiadb sdb = null;
	private String rgName = "rg10225";
	private Random random = new Random();
	private int number = 3;
	private int msec = 1000;
	
	@BeforeClass
	public void setUp(){
		//start time
		System.out.println("Begin to run " + getClass().getName() 
					+ ", begin in: " + dateFm.format(new Date().getTime()));
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			//judge the mode and group number
			if(CommLib.isStandAlone(sdb) || CommLib.OneGroupMode(sdb)){
				throw new SkipException("The mode is standlone, or only one group, skip the testCase.");
			}
			CommLib.clearGroup(sdb, rgName);
		}catch(BaseException e){
			sdb.disconnect();
			Assert.fail(e.getMessage());
		}
	}
	
	@AfterClass
	public void tearDown(){
		try{
			CommLib.clearGroup(sdb, rgName);
		}catch(BaseException e){
			Assert.fail(e.getMessage());
		}finally{
			System.out.println("End to run " + getClass().getName() 
						+ ", end in: " + dateFm.format(new Date().getTime()));
			sdb.disconnect();
		}
	}
	
	@Test(invocationCount = 1, threadPoolSize = 1)
	public void test(){
		
		CreateRG createRG = new CreateRG();
		createRG.start();
		
		RemoveRG removeRG = new RemoveRG();
		CommLib.sleep(random.nextInt(msec));
		removeRG.start();
		
		if( !( createRG.isSuccess() && removeRG.isSuccess() ) ){
			Assert.fail(createRG.getErrorMsg() + removeRG.getErrorMsg());
		}
		
		//check results
		CommLib.checkRGOfCatalog(rgName);
	}

	private class CreateRG extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				db.createReplicaGroup(rgName + "_" + random.nextInt(number));
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -153){
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}

	private class RemoveRG extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				String rgName2 = rgName + "_"  + random.nextInt(number);
				db.removeReplicaGroup(rgName2);
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -154){
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}
	
}