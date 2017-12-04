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

import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
* FileName: TestLobAutoSplit7845.java
* test content:set AutoSplit of cs ,then write lobs  
* testlink case:seqDB-7845
* @author wuyan
    * @Date    2016.9.12
* @version 1.00
*/
public class TestLobAutoSplit7845 extends SdbTestBase {	
	
	private String domainName = "domain7845";
	private String csName = "cs_lob7845";
	private String clName = "cl_lob7845";
	private static Sequoiadb sdb = null;
	private CollectionSpace cs = null;
	private DBCollection cl = null; 
	private Random random = new Random();    
	
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
	}		
		
	private void  createDomain(int groupsNum){
		try{			
			BSONObject options = new BasicBSONObject();
			options = (BSONObject)JSON.parse("{'Groups': [" 
					+ LobUtils.chooseDataGroups(sdb,groupsNum) + "],AutoSplit:true}");
			sdb.createDomain(domainName, options);			
		}catch(BaseException e){
			Assert.assertTrue(false,"create domain fail:"+e.getErrorCode()+e.getMessage());
		}		
		
		//create cs and cl				
		try
		{
			if (sdb.isCollectionSpaceExist(csName)){
				sdb.dropCollectionSpace(csName);
			}
			BSONObject optionsCs = new BasicBSONObject();
			optionsCs = (BSONObject)JSON.parse("{Domain:'" + domainName + "'}");
			cs = sdb.createCollectionSpace(csName, optionsCs);
						
			BSONObject optionsCl = new BasicBSONObject();
			optionsCl = (BSONObject)JSON.parse("{ShardingKey:{a:1,b:-1},ShardingType:'hash',Partition:4096},AutoSplit:true");
			cl = cs.createCollection(clName, optionsCl);
		}catch(BaseException e){
			Assert.assertTrue(false,"create cs/cl fail:csName:"+csName+e.getErrorCode()+e.getMessage());
		}
	}	
	
	public void putLob(){
	    int lobsize = random.nextInt(1048576);
		String lobSb = LobUtils.getRandomString(lobsize);
		ObjectId oid  = null;
		String prevMd5 = "";
		DBLob lob = null;
		try{			
			lob = cl.createLob();
			lob.write(lobSb.getBytes());
		
			prevMd5 = LobUtils.getMd5(lobSb);
		    oid = lob.getID();			
		}catch(BaseException e){
			Assert.assertTrue(false,"pubLob fail:"+e.getMessage());
		}finally{
			if (lob != null){
				lob.close();
			}
		}			
		
		DBLob rLob =null;
		try
		{
			rLob = cl.openLob(oid);
			byte[] rbuff = new byte[1024];
			int readLen =0;
		
			ByteBuffer bytebuff = ByteBuffer.allocate((int)lobsize);
			while ((readLen = rLob.read(rbuff)) != -1){
				bytebuff.put(rbuff, 0, readLen);				
			}
			bytebuff.rewind();
		
			String curMd5 = LobUtils.getMd5(bytebuff);		
			Assert.assertEquals(prevMd5, curMd5);
		}catch(BaseException e){
			Assert.assertTrue(false,"readLob fail:"+e.getMessage());			
		}finally{
			if (rLob != null){
				rLob.close();
			}
		}		
	}
	
	public void dropDomain(){
		try{
			if(sdb.isCollectionSpaceExist(csName)){
				sdb.dropCollectionSpace(csName);
			}
			if(sdb.isDomainExist(domainName)){
				sdb.dropDomain(domainName);
			}			
		}catch(BaseException e){			
			Assert.assertTrue(false,"drop domain/cs failed:"+e.getErrorCode()+e.getMessage());
		}		
	}
	
	@AfterClass
	public void tearDown(){		
		try{			
			sdb.disconnect();
		}catch(BaseException e){			
			Assert.assertTrue(false,"clean up failed:"+e.getMessage());
		}finally{
		}
	}		
	
	@Test
	public void testAutoSplitLob(){			
		try{
			int splitGroupNum = 2;
			createDomain(splitGroupNum);
			putLob();
			dropDomain();
		}catch(BaseException e){
			Assert.assertFalse(true, e.getMessage());
		}
	}
}

