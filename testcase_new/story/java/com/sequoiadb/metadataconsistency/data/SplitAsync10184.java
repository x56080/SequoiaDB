package com.sequoiadb.metadataconsistency.data;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
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
* TestLink: seqDB-10184: concurrency[splitAsync, dropCS]
* @author xiaoni huang init
* @Date   2016.10.24
*/

public class SplitAsync10184 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private static Sequoiadb sdb = null;
	private static ArrayList<String> groupNames = null;
	private String csName = "cs10184_splitAsync";
	private String clName = "cl10184";
	private Random random = new Random();
	private int msec = 300;
	
	@BeforeClass
	public void setUp(){
		//start time
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			//judge the mode or group number or node number
			if(MetaDataUtils.isStandAlone(sdb) || MetaDataUtils.OneGroupMode(sdb)
					|| MetaDataUtils.oneCataNode(sdb) || MetaDataUtils.oneDataNode(sdb)){
				throw new SkipException("The mode is standlone or only one group or one node, "
						+ "skip the testCase.");
			}
			//get groupNames
			groupNames = MetaDataUtils.getDataGroupNames(sdb);
			
			MetaDataUtils.clearCS(sdb, csName);
			
			sdb.createCollectionSpace(csName);
			createCL(sdb, groupNames.get(0));
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
			Assert.fail("ErrorMsg:\n" +e.getMessage());
		}finally{
			sdb.disconnect();
		}
	}
	
	@Test
	public void test(){
		SplitAsync splitAsync = new SplitAsync();
		splitAsync.start();

		DropCS dropCS = new DropCS();
		MetaDataUtils.sleep(random.nextInt(msec));
		dropCS.start();
		
		if( !( splitAsync.isSuccess() && dropCS.isSuccess() ) ){
			Assert.fail(splitAsync.getErrorMsg() + dropCS.getErrorMsg());
		}
		
		//check results
		MetaDataUtils.checkCLResult(csName, clName);
	}
	
	private class SplitAsync extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
					
				if(db.getCollectionSpace(csName).isCollectionExist(clName)){
					DBCollection clDB = db.getCollectionSpace(csName).getCollection(clName);
					BSONObject strCond = new BasicBSONObject();
					BSONObject endCond = new BasicBSONObject();
					strCond.put("a", 0);
					endCond.put("a", 50);
					//System.out.println("split condition: " + strCond + ", " +endCond );
					clDB.splitAsync(groupNames.get(0), groupNames.get(1), strCond, endCond);
				}
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -175 //-175:The mutex task already exist
						&& eCode != -147 && eCode != -23 && eCode != -34 ){  
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
			Sequoiadb db = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				db.dropCollectionSpace(csName);
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -147 && eCode != -34){ 
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}
	
	private void createCL(Sequoiadb sdb, String rgName){
		try{
			CollectionSpace csDB = sdb.getCollectionSpace(csName);
			BSONObject opt = new BasicBSONObject();
			BSONObject subObj = new BasicBSONObject();
			subObj.put("a", 1);
			opt.put("ShardingType", "range");
			opt.put("ShardingKey", subObj);
			opt.put("Group", rgName);
			opt.put("ReplSize", 0);
			csDB.createCollection(clName, opt);
		}catch(BaseException e){
			throw e;
		}
	}
		
}