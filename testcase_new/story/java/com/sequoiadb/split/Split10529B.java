package com.sequoiadb.split;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
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
 * @FileName:SEQDB-10529 切分过程中修改CL :1、向cl中循环插入数据记录 2、执行split，设置范围切分条件
 *                       3、切分过程中执行修改CL操作，分别在如下阶段修改CL:
 *                       a、任务已下发还未开始执行（如执行split后，通过listTasks查看无任务，在此过程中修改cl）
 *                       b、迁移数据过程中（如直连目标组节点查看数据持续插入，可count查询数据量在增加，修改cl中副本数）
 *                       c、目标组更新编目信息后删除cs（如直连目标组查看数据已迁移完成，或者直连编目节点查看cl信息中存在目标组，
 *                       修改cl） 4、查看切分和修改cl操作结果
 *                       备注：此CALSS验证B;修改分区表Shardingtype，Partition会报-6，为合理报错，
 *                        问题单：报-6错误：1697,死锁：2229(已修复)
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10529B extends SdbTestBase {
	private String clName = "testcaseCL_10529B";
	private String srcGroupName;
	private String destGroupName;
	private Sequoiadb commSdb = null;
	private List<BSONObject> insertedData = new ArrayList<BSONObject>();
	private AtomicBoolean flag = new AtomicBoolean(false);
	private String customCSName = "testcaseCS_10529B";

	@BeforeClass
	public void setUp() {

		try {
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

			CollectionSpace cs = commSdb.createCollectionSpace(customCSName);
			DBCollection cl = cs.createCollection(clName, (BSONObject) JSON
					.parse("{ShardingKey:{'sk':1},ReplSize:1,ShardingType:'range',Group:'" + srcGroupName + "'}"));
			insertData(cl);// 写入待切分的记录（1000）
		} catch (BaseException e) {
			if (commSdb != null) {
				commSdb.disconnect();
			}
			Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage() + "\r\n"
					+ SplitUtils.getKeyStack(e, this));
		}
	}

	public void insertData(DBCollection cl) {
		try {
			for (int i = 0; i < 1000; i++) {
				BSONObject obj = (BSONObject) JSON.parse("{sk:" + i + "}");
				cl.insert(obj);
				insertedData.add(obj);
			}

		} catch (BaseException e) {
			throw e;
		}

	}

	@Test(timeOut = 30 * 60 * 1000)
	public void alterCL() {
		Sequoiadb db = null;
		Sequoiadb dataNode = null;
		Split splitThread = null;
		try {

			// 启动切分线程
			splitThread = new Split();
			splitThread.start();

			// 等待目标组数据上涨
			db = new Sequoiadb(coordUrl, "", "");
			db.setSessionAttr((BSONObject) JSON.parse("{PreferedInstance:'M'}"));
			CheckReplSize(db, 1);
			dataNode = db.getReplicaGroup(destGroupName).getMaster().connect();// 获得目标组主节点链接
			while (dataNode.isCollectionSpaceExist(customCSName) != true && flag.get() == false) {
			}

			CollectionSpace cs = dataNode.getCollectionSpace(customCSName);
			while (cs.isCollectionExist(clName) != true && flag.get() == false) {
			}
			DBCollection destCL = dataNode.getCollectionSpace(customCSName).getCollection(clName);
			while (destCL.getCount() == 0 && flag.get() == false) {
			}
			// 修改CL
			DBCollection cl = db.getCollectionSpace(customCSName).getCollection(clName);
			cl.alterCollection((BSONObject) JSON.parse("{ReplSize:3}"));
			CheckReplSize(db, 3);
		} catch (BaseException e) {
			Assert.fail(e.getMessage() + "\r\n" + SplitUtils.getKeyStack(e, this));
			Assert.assertEquals(splitThread.isSuccess(), true, splitThread.getErrorMsg());
		} finally {
			if (splitThread != null) {
				splitThread.join();
			}
			if (db != null) {
				db.disconnect();
			}
			if (dataNode != null) {
				dataNode.disconnect();
			}
		}
	}

	private void CheckReplSize(Sequoiadb db, int size) {
		DBCursor cursor = null;
		try {
			cursor = db.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG, "{Name:\"" + customCSName + "." + clName + "\"}", null,
					null);
			List<BSONObject> tmp = new ArrayList<BSONObject>();
			while (cursor.hasNext()) {
				tmp.add(cursor.getNext());
			}
			Assert.assertEquals(tmp.size(), 1, tmp.toString());
			Assert.assertEquals((int) (tmp.get(0).get("ReplSize")), size, tmp.get(0).toString());
		} catch (BaseException e) {
			Assert.fail(e.getMessage() + "\r\n" + SplitUtils.getKeyStack(e, this));
		} finally {
			if (cursor != null) {
				cursor.close();
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
				cl.split(srcGroupName, destGroupName, (BSONObject) JSON.parse("{sk:100}"), // 切分
						(BSONObject) JSON.parse("{sk:1000}"));
				// 期望目标组有900条符合{sk:{$gte:100,$lt:1000}}查询条件的数据,期望目标组共有900条数据
				checkGroupData(sdb, 900, "{sk:{$gte:100,$lt:1000}}", 900, destGroupName);
				// 校验源组
				checkGroupData(sdb, 100, "{sk:{$gte:0,$lt:100}}", 100, srcGroupName);
			} catch (Exception e) {
				throw e;
			} finally {
				if (sdb != null) {
					sdb.disconnect();
				}
				flag.set(true);
			}
		}
	}

	private void checkGroupData(Sequoiadb sdb, int expectedCount, String macher, int expectTotalCount, String groupName)
			throws Exception {
		Sequoiadb dataNode = null;
		DBCursor cusor = null;
		try {
			dataNode = sdb.getReplicaGroup(groupName).getMaster().connect();// 获得目标组主节点链接
			DBCollection cl = dataNode.getCollectionSpace(customCSName).getCollection(clName);
			cusor = cl.query();
			while (cusor.hasNext()) {
				BSONObject obj = cusor.getNext();
				if (!insertedData.contains(obj)) {
					throw new Exception("inserted data can not find this record:" + obj);
				}
				insertedData.remove(obj);
			}
			long count = cl.getCount(macher);
			if (count != expectedCount) {// 目标组应当含有上述查询数据
				throw new Exception(
						groupName + " getCount(" + macher + "):expected " + expectedCount + " but found " + count);
			}

			if (cl.getCount() != expectTotalCount) {// 目标组应当含有的数据量
				throw new Exception(
						groupName + " getCount:expected " + expectTotalCount + " but found " + cl.getCount());
			}
		} catch (Exception e) {
			throw e;
		} finally {
			if (dataNode != null) {
				dataNode.disconnect();
			}
		}
	}
}
