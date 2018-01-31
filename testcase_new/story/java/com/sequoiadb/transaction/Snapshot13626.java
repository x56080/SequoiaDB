package com.sequoiadb.transaction;

import java.util.List;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbConfTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:TestSnapshot,并发事务过程中查询快照
 * @author luweikang
 * @version 1.00
 */

public class Snapshot13626 extends SdbConfTestBase {
	
	private String clName = "Snapshot13626";
	private Sequoiadb sdb = null;
	private String master = null;
	
	@Override
    protected void setNodeConf(){
        dataConf.put("transactionon", true);
        stdalnConf.put("transactionon", true);
    }
	
	@BeforeClass
	public void setUp(){
		sdb = new Sequoiadb(coordUrl, "", "");
		// 跳过 standAlone 和数据组不足的环境
		CommLib commlib = new CommLib();
		if (commlib.isStandAlone(sdb)) {
			throw new SkipException("skip StandAlone");
		}
		List<String> groupsName = commlib.getDataGroupNames( sdb );
		if (groupsName.size() < 1) {
			throw new SkipException("current environment less than tow groups ");
		}
		ReplicaGroup group = sdb.getReplicaGroup(groupsName.get(0));
		sdb.getCollectionSpace( SdbTestBase.csName ).createCollection( clName ,new BasicBSONObject("Group", groupsName.get(0)));
		String host = group.getMaster().getHostName();
		int port = group.getMaster().getPort();
		master = host + ":" + port;
	}
	
	@Test
	public void test() throws InterruptedException{
		TransactionThread transaction = new TransactionThread();
		transaction.start(100);
		Thread.sleep(5000);
		
		SnapshotThread snapshot = new SnapshotThread();
		snapshot.start(20);
		
		Assert.assertTrue(transaction.isSuccess(), transaction.getErrorMsg());
		Assert.assertTrue(snapshot.isSuccess(), snapshot.getErrorMsg());
	}
	
	@AfterClass
	public void tearDown(){
		sdb.getCollectionSpace(SdbTestBase.csName).dropCollection(clName);
	}
	
	class TransactionThread extends SdbThreadBase {

		@Override
		public void exec() throws Exception {
			Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl,"","");
			DBCollection cl = db.getCollectionSpace(SdbTestBase.csName).getCollection(clName);
			for(int i=0; i<70; i++){
				db.beginTransaction();
				for(int j=0; j<10; j++){
					cl.insert(new BasicBSONObject("a", i));
				}
				db.rollback();
			}
		}
	}
	
	class SnapshotThread extends SdbThreadBase {

		@Override
		public void exec() throws Exception {
			Sequoiadb data = new Sequoiadb( master, "", "" );
			for (int i = 0; i < 500; i++) {
				DBCursor cursor = data.getSnapshot(data.SDB_SNAP_TRANSACTIONS, "", "", "");
				while(cursor.hasNext()){
					cursor.getNext().toString();
				}
				cursor.close();
			}
		}
		
	}
	
}
