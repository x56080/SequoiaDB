package com.sequoiadb.lob;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import java.nio.ByteBuffer;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.SkipException;


import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
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
* @version 1.00
*/

public class TestLobSplit7846 extends SdbTestBase {	
	
	private String clName = "cl_lob7846";
	private static Sequoiadb sdb = null;
	private CollectionSpace cs = null;
	private DBCollection cl = null; 
	private Random random = new Random();
	String sourceRGName = "";
	String targetRGName = "";
	
	@BeforeClass
	public void setUp(){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+SdbTestBase.coordUrl+e.getMessage());
		}
		if (LobUtils.isStandAlone(sdb)){
			throw new SkipException("is standalone skip testcase");
		}
		
		if (LobUtils.OneGroupMode(sdb)){
			throw new SkipException("less two groups skip testcase");
		}
		
		createCL();
	}
			
	//create cs ,then create hashcl
	public void createCL(){
		try{
			if (!sdb.isCollectionSpaceExist(SdbTestBase.csName)){
				sdb.createCollectionSpace(SdbTestBase.csName);	
			}
		}catch(BaseException e){
			//-33 CS exist,ignore exceptions
			Assert.assertEquals(-33,e.getErrorCode(),e.getMessage());
	    }			
		try
		{
			cs = sdb.getCollectionSpace(SdbTestBase.csName);			
			BSONObject options = new BasicBSONObject();
			options = (BSONObject)JSON.parse("{ShardingKey:{a:1,b:-1},ShardingType:'hash',Partition:4096}");
			cl = cs.createCollection(clName, options);			
		}catch(BaseException e){	
			Assert.assertTrue(false,"create cl fail "+e.getErrorType()+":"+e.getMessage());
		}
	}	
	
	public void putLob(){
		long lobNums = 10;
		for(long i = 0; i < lobNums; i++){
			int lobsize = random.nextInt(1048576);
			String lobSb = LobUtils.getRandomString(lobsize);
			String prevMd5 = "";
			ObjectId oid  = null;
			DBLob lob = null;
			try{			
				lob = cl.createLob();
				lob.write(lobSb.getBytes());
			    oid = lob.getID();
			    prevMd5 = LobUtils.getMd5(lobSb);
			}catch(BaseException e){	
				Assert.assertTrue(false,"write lob fail:"+e.getMessage()+e.getStackTrace());
			}finally{
				if (lob != null){
					lob.close();
				}
			}
			
			DBLob rLob = null;
			try
			{
				rLob = cl.openLob(oid);			
				int rbuffSize = 1024;
				byte[] rbuff = new byte[rbuffSize];
				int readLen =0;		
				ByteBuffer bytebuff = ByteBuffer.allocate((int)lobsize);
				while ((readLen = rLob.read(rbuff)) != -1){
					bytebuff.put(rbuff, 0, readLen);				
				}
				bytebuff.rewind();		
				String curMd5 = LobUtils.getMd5(bytebuff);
				Assert.assertEquals(curMd5, prevMd5,"the lobs md5 different");
			}catch(BaseException e){
				Assert.assertTrue(false,"read lob fail:"+e.getMessage()+e.getStackTrace());
			}finally{
				if (rLob != null){
					rLob.close();
				}
			}			
		}	    
	}		
		
	public void splitCL(){		
		try{
			BSONObject cond = new BasicBSONObject();
			BSONObject endCond = new BasicBSONObject();
			cond.put("Partition", 1024);
			endCond.put("partition", 3072);
			sourceRGName = LobUtils.getSrcGroupName(sdb,SdbTestBase.csName,clName);
			targetRGName = LobUtils.getSplitGroupName(sourceRGName);
			cl.split(sourceRGName, targetRGName, cond, endCond);
		}catch(BaseException e){
			Assert.assertTrue(false,"split fail:"+e.getMessage()+"srcRGName:"+sourceRGName
					+"\n tarRGName:"+targetRGName);
		}		
	}	
	
	public void checkSplitResult(){			
		DBCursor listCursor = cl.listLobs();
		int count = 0;
		while ( listCursor.hasNext() ) {			
			count++;
			listCursor.getNext();		
		}		
		listCursor.close();	
		
		int allCount = 0;
		Sequoiadb dataDB = null;
		BasicBSONList splitGroupNames = new BasicBSONList();
		splitGroupNames.add(sourceRGName);
		splitGroupNames.add(targetRGName);		
		for(int i=0; i< splitGroupNames.size();i++){
			try{
				String nodeName = sdb.getReplicaGroup((String)splitGroupNames.get(i)).getMaster().getNodeName();	
				dataDB = new Sequoiadb(nodeName,"","");			
				DBCollection dataCL = dataDB.getCollectionSpace(SdbTestBase.csName).getCollection(clName);
				DBCursor listLobs = dataCL.listLobs();
				int subCount = 0;				
				while ( listLobs.hasNext() ) {					
					subCount++;
					listLobs.getNext();						
				}
				listLobs.close();	
				allCount += subCount;				
				//list lobs deviation value is less than 50 percent after split
				/*double errorValue = Math.abs(count*0.5 - subCount)/(count*0.5);
				if (errorValue > 0.5){
					Assert.assertTrue(false,"large number of partitions,errorValue: "+errorValue);					
				}*/				
			}catch(BaseException e){
				Assert.assertTrue(false,"check split result fail " + e.getErrorCode()+e.getMessage());
			}finally{
				if (dataDB != null){
					dataDB.disconnect();
				}
			}
		}
		//sum of query results on each group is equal to the results of coord 
		Assert.assertEquals(allCount,count,"list lobs error."+"allCount:"+allCount);			
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
		}
	}	
	
	@Test
	void testSplitLob(){	
		try{			
			splitCL();
			putLob();
			checkSplitResult();
		}catch(BaseException e){
			Assert.assertFalse(true, e.getMessage());
		}
	}
}
