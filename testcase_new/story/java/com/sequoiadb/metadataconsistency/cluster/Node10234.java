package com.sequoiadb.metadataconsistency.cluster;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.AfterClass;
import org.testng.Assert;
import org.testng.SkipException;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadataconsistency.data.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
* TestLink: seqDB-10234:concurrency[attachNode, detachNode]
* @author xiaoni huang init
* @Date   2016.10.24
*/

public class Node10234 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private Random random = new Random();
	private static Sequoiadb sdb = null;
	private String rgName = "rg10234";
	private int msec = 100;
	
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

			ReplicaGroup rg = sdb.createReplicaGroup(rgName);
			createNode();
			rg.start();
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
	
	@Test(invocationCount = 3, threadPoolSize = 3)
	public void test(){

		DetachNode detachNode = new DetachNode();
		detachNode.start();
		
		AttachNode attachNode = new AttachNode();
		CommLib.sleep(random.nextInt(msec));
		attachNode.start();
		
		if( !( detachNode.isSuccess() && attachNode.isSuccess() ) ){
			Assert.fail(detachNode.getErrorMsg() + attachNode.getErrorMsg());
		}
		
		//check results
		CommLib.checkRGOfCatalog(rgName);
	}

	private class DetachNode extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				
				ReplicaGroup rgDB = db.getReplicaGroup(rgName);
				String hostName = rgDB.getSlave().getHostName();
				int svcName = rgDB.getSlave().getPort();
				
				rgDB.detachNode(hostName, svcName, null);
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -204 //-204:Unable to remove the last node or primary in a group
						&& eCode != -155){ //-155:Node does not exist(node has been to detach)
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}

	private class AttachNode extends SdbThreadBase{
		@Override
		public void exec() throws BaseException{
			Sequoiadb db = null;
			try
			{
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				
				ReplicaGroup rgDB = db.getReplicaGroup(rgName);
				String hostName = rgDB.getSlave().getHostName();
				int svcName = rgDB.getSlave().getPort();
				rgDB.attachNode(hostName, svcName, null);
			}catch(BaseException e){
				int eCode = e.getErrorCode();
				if( eCode != -157 && eCode != -155){ //-157:Invalid node configuration (node has been to attach)
					throw e;
				}
			}finally{
				db.disconnect();
			}
		}
	}
	
	public void createNode(){
		try
		{
			for(int i = 0; i < 3; i++){
				CommLib.createNode( sdb, rgName, 
						   SdbTestBase.reservedPortBegin, 
						   SdbTestBase.reservedPortEnd, 
						   SdbTestBase.reservedDir );
			}
		}catch(BaseException e){
			throw e;
		}
	}
	
}