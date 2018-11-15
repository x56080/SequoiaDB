package com.sequoiadb.lob;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.SkipException;

import java.util.ArrayList;
import java.util.Random;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingDeque;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
* FileName: TestLobSplit7846.java
* test content:lob split
* testlink case:seqDB-7846
* @author wuyan
    * @Date    2016.9.12
    * @update  [2017.12.20]
* @version 1.00
*/

public class TestLobSplit7846 extends SdbTestBase {	
	
	private String clName = "cl_lob7846";
	private static Sequoiadb sdb = null;
	private CollectionSpace cs = null;
	private DBCollection cl = null; 
	private Random random = new Random();
	private ConcurrentHashMap<ObjectId, String> id2md5 
    							= new ConcurrentHashMap<ObjectId, String>();
	private LinkedBlockingDeque<ObjectId> oidQueue = new LinkedBlockingDeque<ObjectId>();
	private ArrayList<String> splitRGList = new ArrayList<String>(2);
	private String sourceRGName = "";
	private String targetRGName = "";
	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+SdbTestBase.coordUrl+e.getMessage());
		}
		sdb.setSessionAttr( (BSONObject) JSON.parse("{'PreferedInstance':'M'}"));
		if (LobOprUtils.isStandAlone(sdb)){
			throw new SkipException("is standalone skip testcase");
		}
		
		if (LobOprUtils.OneGroupMode(sdb)){
			throw new SkipException("less two groups skip testcase");
		}
		
		
		createCL();
		int lobNums = 100;
		writeLobAndGetMd5(lobNums);
	}
			
	@Test
	void testSplitLob(){	
		try{	
			//test a:split by cond
			splitCLByCond();			
			double expErrorValue = 0.5;				
			splitRGList.add(sourceRGName);
			splitRGList.add(targetRGName);
			LobOprUtils.checkSplitResult(sdb, csName, clName, splitRGList, expErrorValue);
			
			//test b:split by percent,eg:split from targetRG to srcRG
			splitCLByPercent();
			//compare the total lobnums ,expect error value is 0.9
			double expErrorValue1 = 0.95;		
			LobOprUtils.checkSplitResult(sdb, csName, clName, splitRGList, expErrorValue1);
			
			//testpoint a and b is contain the testpoint c ,check the lobdata after split
			int lobNums = 100;
			readLobAndCheckMd5(lobNums);

		}catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	
		
	private void splitCLByCond(){	
		try{
			BSONObject cond = new BasicBSONObject();
			BSONObject endCond = new BasicBSONObject();
			cond.put("Partition", 2048);
			endCond.put("partition", 4096);
			sourceRGName = LobOprUtils.getSrcGroupName(sdb,SdbTestBase.csName,clName);
			targetRGName = LobOprUtils.getSplitGroupName(sourceRGName);
			cl.split(sourceRGName, targetRGName, cond, endCond);
		}catch(BaseException e){
			Assert.assertTrue(false,"split fail:"+e.getMessage()+"srcRGName:"+sourceRGName
					+"\n tarRGName:"+targetRGName);
		}		
	}	
	
	private void splitCLByPercent(){	
		try{
			int percent = 80;
			cl.split(targetRGName, sourceRGName, percent);
		}catch(BaseException e){
			Assert.assertTrue(false,"split fail:"+e.getMessage()+"srcRGName:"+sourceRGName
					+"\n tarRGName:"+targetRGName);
		}		
	}	
	
	

	@AfterClass
	public void tearDown(){		
		try{			
			if(cs.isCollectionExist(clName)){
				cs.dropCollection(clName);
			}
			sdb.disconnect();
		}catch(BaseException e){			
			Assert.assertTrue(false,"clean up failed:"+e.getMessage());
		}finally{
			if( null != sdb){
				sdb.disconnect();
			}
		}
	}	
	
	
	
	//create cs ,then create hashcl
	public void createCL(){				
		try
		{
			cs = sdb.getCollectionSpace(SdbTestBase.csName);			
			BSONObject options = new BasicBSONObject();
			options = (BSONObject)JSON.parse("{ShardingKey:{a:1},ShardingType:'hash',Partition:4096}");
			cl = cs.createCollection(clName, options);			
		}catch(BaseException e){	
			Assert.assertTrue(false,"create cl fail "+e.getErrorType()+":"+e.getMessage());
		}
	}	
	
	
	private void writeLobAndGetMd5(int lobtimes){
		for( int i = 0; i< lobtimes; i++){
			int writeLobSize = random.nextInt(1024*1024);;
			byte[] wlobBuff = LobOprUtils.getRandomBytes(writeLobSize);
			ObjectId oid = LobOprUtils.createAndWriteLob(cl, wlobBuff);	
			
			//save oid and md5
			String prevMd5 = LobOprUtils.getMd5(wlobBuff);
			oidQueue.offer(oid);			
			id2md5.put(oid, prevMd5);			
		}		
	}	
	
	private void readLobAndCheckMd5(int lobNums) throws InterruptedException{
		for( int i = 0 ; i < lobNums; i++){
			ObjectId oid = oidQueue.take();	
			DBLob rLob = null;
			try{  
				rLob = cl.openLob(oid);
				byte[] rbuff = new byte[(int) rLob.getSize()];
				rLob.read(rbuff);   
				rLob.close();
				String curMd5 = LobOprUtils.getMd5(rbuff);
				String prevMd5 = id2md5.get(oid);
				Assert.assertEquals(curMd5, prevMd5);
				id2md5.remove(oid);        			
			}finally{
				if( rLob != null ){
					rLob.close();
				}
			}    
		}
		  		
	}	
	
}
