package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-10527 切分过程中删除CS :1、向cl中循环插入数据记录 2、执行split，设置范围切分条件
 *                       3、切分过程中执行删除cs操作，分别在如下阶段删除CS:
 *                       a、任务已下发还未开始执行（如执行split后，通过listTasks查看无任务，在此过程中删除cs）
 *                       b、迁移数据过程中（如直连目标组节点查看数据持续插入，可count查询数据量在增加）
 *                       c、目标组更新编目信息后删除cs（如直连目标组查看数据已迁移完成，或者直连编目节点查看cl信息中存在目标组）
 *                       4、查看切分和删除cs操作结果 备注：验证B场景
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10527B extends SdbTestBase {
	private String clName = "testcaseCL_10527B";
	private String customCSName = "testcaseCS_10527B";
	private String srcGroupName;
	private String destGroupName;
	private Sequoiadb commSdb = null;
	private AtomicBoolean flag = new AtomicBoolean(false);
	private AtomicBoolean errno147 = new AtomicBoolean(false);

	@BeforeClass()
	public void setUp() {
		commSdb = new Sequoiadb(coordUrl, "", "");

		// 跳过 standAlone 和数据组不足的环境
		CommLib commlib = new CommLib();
		if (commlib.isStandAlone(commSdb)) {
			throw new SkipException("skip StandAlone");
		}
		List<String> groupsName = commlib.getDataGroupNames(commSdb);
		if (groupsName.size() < 2) {
			throw new SkipException("current environment less than tow groups ");
		}
		srcGroupName = groupsName.get(0);
		destGroupName = groupsName.get(1);
			
		DBCollection dbcl = createCSAndCL();
		// 写入待切分的记录（30000）
		insertData(dbcl);
	}
	
	@Test(timeOut = 30 * 60 * 1000)
	public void dropCS() {
		Sequoiadb dataNode = null;
		Split splitThread = null;
		try {
			// 启动切分线程
			splitThread = new Split();
			splitThread.start();

			// 等待目标组数据开始上涨
			dataNode = commSdb.getReplicaGroup(destGroupName).getMaster().connect();// 获得目标组主节点链接

			// flag为了防止split线程失败，产生死循环
			while (dataNode.isCollectionSpaceExist(customCSName) != true && flag.get() == false) {
			}
			CollectionSpace cs = dataNode.getCollectionSpace(customCSName);
			while (cs.isCollectionExist(clName) != true && flag.get() == false) {
			}
			DBCollection cl = dataNode.getCollectionSpace(customCSName).getCollection(clName);
			while (cl.getCount() == 0 && flag.get() == false) {				
			}
			
			try{
				//随机在迁移过程中删除CS
				Random random = new Random();
				int sleeptime = random.nextInt(4000);				
				Thread.sleep(sleeptime);
				commSdb.dropCollectionSpace(customCSName);				
				Assert.assertEquals(commSdb.isCollectionSpaceExist(customCSName), false);
			} catch (BaseException e) {
				System.out.println("---e:"+e.getErrorCode());
				if ( e.getErrorCode() != -147){
					Assert.fail(e.getMessage() );
				}	
			}			

			// 检测切分线程
			Assert.assertEquals(splitThread.isSuccess(), true, splitThread.getErrorMsg());

			// 若切分线程-147，检测切分任务是否遗留，以下代码若切分任务存在，则重复检测10s，10s后依然遗留则判断用例失败
			boolean isSplitTaskExist = false;
			BSONObject taskBson = null;
			if (errno147.get()) {
				for (int i = 0; i < 10; i++) {
					DBCursor cusor = commSdb.listTasks((BSONObject) JSON.parse("{Name:'" + customCSName + "." + clName + "'}"), null,
							null, null);
					if (cusor.hasNext()) {
						taskBson = cusor.getNext();
						System.out.println("split faile(-147),but task still exist,now try check again:" + taskBson);
						cusor.close();
					} else {
						isSplitTaskExist = false;
						break;
					}
					Thread.sleep(1000);
				}
				if (isSplitTaskExist) {
					Assert.fail("split faile(-147),but task still exist(at least 10s):"+taskBson);
				}
			}			
		} catch (InterruptedException e) {
			Assert.fail(e.getMessage()+SplitUtils.getKeyStack(e, this));
		} finally {			
			if (dataNode != null) {
				dataNode.disconnect();
			}
			if (splitThread != null) {
				splitThread.join();
			}
		}
	}

	@AfterClass
	public void tearDown() {
		try {
			if (commSdb.isCollectionSpaceExist(customCSName)) {
				commSdb.dropCollectionSpace(customCSName);
			}
		} catch (BaseException e) {
			Assert.fail(e.getMessage() + "\r\n" + SplitUtils.getKeyStack(e, this));
		} finally {
			if (commSdb != null) {
				commSdb.disconnect();
			}
		}
	}

	class Split extends SdbThreadBase {

		@Override
		public void exec() throws Exception {
			Sequoiadb sdb = null;
			try {
				sdb = new Sequoiadb(coordUrl, "", "");
				DBCollection cl = sdb.getCollectionSpace(customCSName).getCollection(clName);						
				cl.split(srcGroupName, destGroupName, 90);				
			} catch (BaseException e) {
				if (e.getErrorCode() != -147) {
					throw e;
				} else {
					errno147.set(true);
				}
			} finally {
				if (sdb != null) {
					sdb.disconnect();
				}
				flag.set(true);
			}
		}
	}
	
	private DBCollection createCSAndCL(){
		if( commSdb.isCollectionSpaceExist(customCSName)){
			commSdb.dropCollectionSpace(customCSName);			
		}
		CollectionSpace customCS = commSdb.createCollectionSpace(customCSName);
		DBCollection cl = customCS.createCollection(clName, (BSONObject) JSON
				.parse("{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'" + srcGroupName + "'}"));
		return cl;		
	}
	
	//insert 3W records
	private void insertData(DBCollection cl) {
		int count = 0;
		for ( int i = 0; i < 3; i++){
			List<BSONObject>list = new ArrayList<BSONObject>();	
			for (int j = 0; j < 10000; j++) {
				int value = count++;
				BSONObject obj = (BSONObject) JSON.parse("{sk:" + value +", test:"+"'testasetatatatatat'" + "}");				
				list.add(obj);	
			}
			cl.bulkInsert(list, 0);
		}		
	}


}
