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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
* FileName: TestLobSplitAndWrite7847.java
* test content:when spliting ,write/read/remove lob  
* testlink cases:seqDB-7847/seqDB-7849
* @author wuyan
    * @Date    2016.10.9
* @version 1.00
*/
public class TestLobSplitAndWrite7847 extends SdbTestBase {
	private String clName = "cl_lob7847";
	private static Sequoiadb sdb = null;
	private CollectionSpace cs = null;	
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
		
		createCL();
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
	public void testSplitAndWrite(){		
		SplitCL splitCL = new SplitCL();
		splitCL.start();
		testWriteAndRemoveLob();
	    if(!splitCL.isSuccess()){
	    	Assert.fail(splitCL.getErrorMsg());
	    } 	    
	}
	
	public class SplitCL extends SdbThreadBase{
		@Override
        public void exec() throws BaseException{
			
            Sequoiadb db1 = null;
            DBCollection cl1 = null;
            String sourceRGName = LobUtils.getSrcGroupName(sdb, SdbTestBase.csName, clName);
			String targetRGName = LobUtils.getSplitGroupName(sourceRGName);
            try{
            	db1 = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            	cl1 = db1.getCollectionSpace(SdbTestBase.csName).getCollection(clName);            	
				BSONObject cond = new BasicBSONObject();
				BSONObject endCond = new BasicBSONObject();
				cond.put("Partition", 512);
				endCond.put("partition", 2048);	
				cl1.split(sourceRGName, targetRGName,cond,endCond);
            }catch(BaseException e){
            	Assert.assertTrue(false,"split fail\n"+"srcGroup:"+sourceRGName
									+"\ntarGroup"+targetRGName+e.getMessage());
            }finally{
            	if (db1 != null){
    				db1.disconnect();
    			}
            }		
		}
	}	
	
	public void testWriteAndRemoveLob(){	
		Sequoiadb db  = null;
		try{			
			db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			BSONObject obj = (BSONObject) JSON.parse("{PreferedInstance:\"M\"}");
			db.setSessionAttr(obj);
			CollectionSpace cs = db.getCollectionSpace(SdbTestBase.csName);
			DBCollection dbcl = cs.getCollection(clName);
			ObjectId oid = putLob(dbcl);
			removeLob(dbcl,oid);
		}catch(BaseException e){
			Assert.assertTrue(false, e.getMessage());
		}finally{
			if (db != null){
				db.disconnect();
			}
		}
	}
	
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
	    	String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:4096,"
					+ "ReplSize:0,Compressed:true}";
		    	BSONObject options =(BSONObject) JSON.parse(clOptions);
		    cs = sdb.getCollectionSpace(SdbTestBase.csName);			
		    cs.createCollection(clName,options);			
	    }catch(BaseException e){
		    Assert.assertTrue(false,"create cl fail "+e.getErrorType()+":"+e.getMessage());
	    }
	 }	
	
	private ObjectId putLob(DBCollection cl){
		long lobNums = 10;
		ObjectId oid  = null;
		for(long i = 0; i < lobNums; i++){
			int lobsize = random.nextInt(1048576);
			String lobSb = LobUtils.getRandomString(lobsize);
			String prevMd5 = "";			
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
		return oid;
	}	
		
	public void removeLob( DBCollection cl,ObjectId oid ){			
		try{
			cl.removeLob(oid);
		}catch(BaseException e){
			Assert.assertTrue(false,"remove lob fail");
		}
		
		boolean IsExistLob = false;
		DBCursor listLob = cl.listLobs();	
		while(listLob.hasNext()){
			BasicBSONObject obj = (BasicBSONObject)listLob.getNext();
			if (obj.getObjectId("Oid").equals(oid)){
				IsExistLob = true;
				break;
			}
		}	
		Assert.assertEquals(IsExistLob,false,"list remove lob not null");
	}	
}
