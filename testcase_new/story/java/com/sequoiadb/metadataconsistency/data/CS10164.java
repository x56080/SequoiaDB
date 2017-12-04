package com.sequoiadb.metadataconsistency.data;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
* 		seqDB-10164: concurrency[drop cs, alter domain]
* @author xiaoni huang init
* @Date   2016.9.26
*/

public class CS10164 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private static Sequoiadb sdb = null;
	private static ArrayList<String> dataGroups = null;
	private String domainName = "dm10164";
	private String csName = "cs10164";
	private Random random = new Random();
	private int number = 20;
	private int msec = 100;
	
	@BeforeClass
	public void setUp(){
		//start time
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			//judge the mode and group number
			if(MetaDataUtils.isStandAlone(sdb)){
				throw new SkipException("The mode is standlone, skip the testCase.");
			}
			MetaDataUtils.clearCS(sdb, csName);
			MetaDataUtils.clearDomain(sdb, domainName);
			
			dataGroups = MetaDataUtils.getDataGroupNames(sdb);
			createDomain(sdb);
		}catch(BaseException e){
			sdb.disconnect();
			Assert.fail(e.getMessage());
		}
	}
	
	@AfterClass
	public void tearDown(){
		try{
			//clear env
			MetaDataUtils.clearCS(sdb, csName);
			MetaDataUtils.clearDomain(sdb, domainName);
		}catch(BaseException e){
			Assert.fail(e.getMessage());
		}finally{
			sdb.disconnect();
		}
	}
	
	@Test(invocationCount = 10, threadPoolSize = 10)
	public void test(){
		DropCS dropCS = new DropCS();
		dropCS.start();
		
		AlterDomain alterDomain = new AlterDomain();
		MetaDataUtils.sleep(random.nextInt(msec));
		alterDomain.start();
		
		if( !( dropCS.isSuccess() && alterDomain.isSuccess() ) ){
			Assert.fail(dropCS.getErrorMsg() + alterDomain.getErrorMsg());
		}

		//check results
		MetaDataUtils.checkDomainOfCatalog(domainName);
		MetaDataUtils.checkCSOfCatalog(csName);
	}
	
	private class DropCS extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				
				String tmpCSName = csName + "_" + random.nextInt(number);
				db.dropCollectionSpace(tmpCSName);
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -34 && eCode != -147 ){
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}
	
	private class AlterDomain extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db  = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				
				BSONObject opt = new BasicBSONObject();
				opt.put( "Groups", dataGroups.get(0).split(",") );
				opt.put( "AutoSplit", false );
				db.getDomain(domainName).alterDomain(opt);
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
	
	public void createDomain(Sequoiadb sdb){
		try{
			BSONObject opt = new BasicBSONObject();
			if(!sdb.isDomainExist(domainName)){
				opt.put( "Groups", dataGroups );
				opt.put( "AutoSplit", true );
			    sdb.createDomain ( domainName, opt);
			}
		}catch(BaseException e){
			throw e;
		}
	}
	
	public void createCS(Sequoiadb sdb){
		try{
			for(int i = 0; i< number; i++){
				BSONObject opt = new BasicBSONObject();
				opt.put( "Domain", domainName );
				sdb.createCollectionSpace(csName+i, opt);
			}
		}catch(BaseException e){
			throw e;
		}
	}
		
}