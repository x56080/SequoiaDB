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
import java.util.concurrent.LinkedBlockingDeque;


import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
* FileName: TestLobSplitAndWrite7848.java
* test content:cl1 remove lob and split ,when cl1 writelob  
* testlink cases:seqDB-7848
* @author wuyan
    * @Date    2016.10.9
    * @update  [2017.12.20]
* @version 1.00
*/
public class TestLobSplitAndWrite7848 extends SdbTestBase {	
	private String clName1 = "cl_lob7848a";
	private String clName2 = "cl_lob7848b";
	private String csName = "cs7848";
	private static Sequoiadb sdb = null;
	private Random random = new Random();
	private String sourceRGName = "";
	private String targetRGName = "";
	private byte[] wlobBuff = null;
	private String prevMd5 = "";	
	private LinkedBlockingDeque<ObjectId> oidQueue1 = new LinkedBlockingDeque<ObjectId>();
	private LinkedBlockingDeque<ObjectId> oidQueue2 = new LinkedBlockingDeque<ObjectId>();
	
	@BeforeClass
	public void setUp(){
	
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+SdbTestBase.coordUrl+e.getMessage());
		}
		if (CommLib.isStandAlone(sdb)){
			throw new SkipException("is standalone skip testcase");
		}
		
		if (CommLib.OneGroupMode(sdb)){
			throw new SkipException("less two groups skip testcase");
		}
				
		createCSAndCL();
		sdb.setSessionAttr( (BSONObject) JSON.parse("{'PreferedInstance':'M'}"));
		//cl1 write lob 
		DBCollection dbcl1 = sdb.getCollectionSpace(csName).getCollection(clName1);
		int lobtimes = 100;
		int writeLobSize = random.nextInt(1024*1024);		
		wlobBuff = LobOprUtils.getRandomBytes(writeLobSize);
		prevMd5 = LobOprUtils.getMd5(wlobBuff);
        writeLob(dbcl1, lobtimes, oidQueue1); 
	}	
	
	@Test
	public void testSplitAndWrite(){
		//cl2(cl_lob7848b) writeLob operation,write lob nums :30 *4 =120
		PutLobsTask putLobTasks = new PutLobsTask();
		putLobTasks.start(30);
		
		//cl1(cl_lob7848a) removeLob and split operation:remove lob nums is 80
		RemoveLobsTask removeLobsTasks = new RemoveLobsTask();
		removeLobsTasks.start(80);
		
		SplitCL splitCL = new SplitCL();
		splitCL.start();
		
	    if(!splitCL.isSuccess()){
	    	Assert.fail(splitCL.getErrorMsg());
	    } 	    
	    Assert.assertTrue( putLobTasks.isSuccess(), putLobTasks.getErrorMsg());
	    Assert.assertTrue( putLobTasks.isSuccess(), putLobTasks.getErrorMsg());
	    Assert.assertTrue( removeLobsTasks.isSuccess(), removeLobsTasks.getErrorMsg());
	    
	    //cl1 check the split result	    
	    ArrayList<String> splitRGNames = new ArrayList<String>(2);
	    splitRGNames.add(sourceRGName);
	    splitRGNames.add(targetRGName);
		checkSplitResult(sdb, csName, clName1, splitRGNames);	
		//check the lob data
		checkLobofCL( clName1, oidQueue1);
		checkLobofCL( clName2, oidQueue2);
	}
	
	@AfterClass
	public void tearDown(){		
		try{			
			if(sdb.isCollectionSpaceExist(csName)){
				sdb.dropCollectionSpace(csName);;
			}
			sdb.disconnect();
		}catch(BaseException e){			
			Assert.assertTrue(false,"clean up failed:"+e.getMessage());
		}finally{
			if ( null != sdb ){
				sdb.disconnect();
			}
		}
	}	
	
	public class SplitCL extends SdbThreadBase{
		@Override
        public void exec() throws BaseException{            
            sourceRGName = LobOprUtils.getSrcGroupName(sdb, csName, clName1);
			targetRGName = LobOprUtils.getSplitGroupName(sourceRGName);
			
			Sequoiadb db1 = null;
            try{
            	db1 = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            	DBCollection cl1 = db1.getCollectionSpace(csName).getCollection(clName1);            	
				int percent = 50;
				cl1.split(sourceRGName, targetRGName,percent);
            }catch(BaseException e){
            	Assert.assertTrue(false,"split fail\n"+"srcGroup:"+sourceRGName
									+"\ntarGroup"+targetRGName+e.getMessage());
            }finally {
            	if ( db1 != null ){
            		db1.disconnect();
            	} 
			}
		}
	}		
			
	private class RemoveLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException, InterruptedException{
        	Sequoiadb db = null;
            try{   
            	db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
                DBCollection dbcl = db.getCollectionSpace(csName).getCollection(clName1);                 
                ObjectId oid = oidQueue1.take();
                dbcl.removeLob(oid);               
        	}finally{
        		if ( db != null ){
        			db.disconnect();
        		}
        	}          
        }
    }
	
	private class PutLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException{
        	Sequoiadb db = null;
            try{ 
            	db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
                DBCollection dbcl = db.getCollectionSpace(csName).getCollection(clName2);                 
                int lobtimes = 4;  
                writeLob(dbcl, lobtimes, oidQueue2);
            }finally{
            	if ( db != null ){
            		db.disconnect();
            	}
            }
        }
    }
				
	private void checkLobofCL(String clName, LinkedBlockingDeque<ObjectId> oidQueue ){
		int count = 0;
		DBCollection dbcl = sdb.getCollectionSpace(csName).getCollection(clName);
		DBCursor listLob = null;
		try{
			listLob = dbcl.listLobs();
			while(listLob.hasNext()){
				BasicBSONObject obj = (BasicBSONObject)listLob.getNext();
				ObjectId existOid = obj.getObjectId("Oid");	
				Assert.assertEquals(oidQueue.contains(existOid),true,existOid.toString()+" of "+clName+" is not found in oidQueue!");
				
				DBLob rLob = dbcl.openLob(existOid);
				byte[] rbuff = new byte[(int) rLob.getSize()];
				rLob.read(rbuff);
				rLob.close();
				String curMd5 = LobOprUtils.getMd5(rbuff);        			
        		Assert.assertEquals(curMd5, prevMd5); 		
				count++;
			}
		}finally{
			if ( listLob != null ){
				listLob.close();				
			}
		}
		
		//the list lobnums must be consistent with the number of remaining digits in the actual oidqueue
		Assert.assertEquals(count, oidQueue.size());
	}	
	
	
		
	private void writeLob(DBCollection cl, int lobtimes, LinkedBlockingDeque<ObjectId> oidQueue ){		
		for( int i = 0; i< lobtimes; i++){			
			ObjectId oid = LobOprUtils.createAndWriteLob(cl, wlobBuff);				
			//save oid			
			oidQueue.offer(oid);			
		}		
	}	
	
	
	public void createCSAndCL(){
		if( sdb.isCollectionSpaceExist(csName)){
			sdb.dropCollectionSpace(csName);
		}
		CollectionSpace cSpace = sdb.createCollectionSpace(csName);
		String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:4096,"
				+ "ReplSize:0,Compressed:true}";
		BSONObject options =(BSONObject) JSON.parse(clOptions);	
		cSpace.createCollection(clName1, options);
		cSpace.createCollection(clName2, options);	    
	 }	
	
	private void checkSplitResult(Sequoiadb sdb, String csName, String clName,ArrayList<String> splitGroupNames){	
		DBCollection cl = sdb.getCollectionSpace(csName).getCollection(clName);
		DBCursor listCursor = cl.listLobs();
		int count = 0;
		while ( listCursor.hasNext() ) {			
			count++;
			listCursor.getNext();		
		}		
		listCursor.close();	
		
		int actListNums = 0;		
		for(int i=0; i< splitGroupNames.size();i++){			
			String nodeName = sdb.getReplicaGroup((String)splitGroupNames.get(i)).getMaster().getNodeName();
			Sequoiadb dataDB = null;
			try{	
				dataDB = new Sequoiadb(nodeName,"","");
				DBCollection dataCL = dataDB.getCollectionSpace(csName).getCollection(clName);
				DBCursor listLobs = dataCL.listLobs();
				int subCount = 0;				
				while ( listLobs.hasNext() ) {					
					subCount++;
					listLobs.getNext();						
				}
				listLobs.close();	
				actListNums += subCount;				
			}finally{
				if ( dataDB != null ){
					dataDB.disconnect();
				}
			}
		}
		//sum of query results on each group is equal to the results of coord 
		Assert.assertEquals(actListNums,count,"list lobs error."+"allCount:"+actListNums);			
	}
}
