package com.sequoiadb.split;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

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
 * @FileName:SEQDB-10536 切分过程中删除id索引 1、向cl中插入数据记录，创建ID索引 2、执行split，设置切分条件
 *                       3、切分过程中删除id索引（源组清除数据之前删除id索引） 4、查看切分和删除id索引结果 
 *                       此用例暂未开启,问题单：2203
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10536 extends SdbTestBase {
	private String clName = "testcaseCL_10536";
	private String srcGroupName;
	private String destGroupName;
	private Sequoiadb commSdb = null;
	private List<BSONObject> insertedData = new ArrayList<BSONObject>();

	@BeforeClass(enabled=false)
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

			CollectionSpace customCS = commSdb.getCollectionSpace(csName);
			customCS.createCollection(clName, (BSONObject) JSON
					.parse("{ShardingKey:{'sk':1},ShardingType:'range',Group:'" + srcGroupName + "'}"));
			insertData();// 写入待切分的记录（500）
		} catch (BaseException e) {
			if (commSdb != null) {
				commSdb.disconnect();
			}
			e.printStackTrace();
			Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		}
	}

	public void insertData() {
		try {
			DBCollection cl = commSdb.getCollectionSpace(csName).getCollection(clName);

			List<BSONObject> tmp = new ArrayList<BSONObject>();
			for (int i = 0; i < 500; i++) {
				BSONObject obj = (BSONObject) JSON.parse("{sk:" + i + "}");
				// cl.insert(obj);
				tmp.add(obj);
			}
			cl.bulkInsert(tmp, 0);
			insertedData.addAll(tmp);

		} catch (BaseException e) {
			throw e;
		}

	}

	@Test(enabled=false)
	public void delteIdIndex() {
		Sequoiadb db = null;
		Split splitThread = null;
		try {
			// 启动切分线程
			splitThread = new Split();
			splitThread.start();

			// 删除ID索引
			db = new Sequoiadb(coordUrl, "", "");
			DBCollection cl = db.getCollectionSpace(csName).getCollection(clName);
			cl.dropIdIndex();

			// 等待切分结束
			if (!splitThread.isSuccess()) {
				splitThread.getExceptions().get(0).printStackTrace();
				Assert.fail(splitThread.getErrorMsg());
			}

			// 查看索引
			checkIndexNonExist(cl);

			// 校验源组和目标组的数据
			checkGroupData(db, 450, "{sk:{$gte:50,$lt:500}}", 450, destGroupName);
			checkGroupData(db, 50, "{sk:{$gte:0,$lt:50}}", 50, srcGroupName);
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (db != null) {
				db.disconnect();
			}
		}
	}

	@AfterClass(enabled=false)
	public void tearDown() {
		try {
			CollectionSpace cs = commSdb.getCollectionSpace(csName);
			cs.dropCollection(clName);
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (commSdb != null) {
				commSdb.disconnect();
			}
		}
	}

	// 检查是否只存在shard索引
	public void checkIndexNonExist(DBCollection cl) {
		DBCursor dbc = null;
		BSONObject shardingKeyIndex = (BSONObject) JSON
				.parse("{name: \"$shard\",key: {sk: 1},v: 0,unique: false,dropDups: false,enforced: false}");
		ArrayList<BSONObject> expect = new ArrayList<BSONObject>();
		expect.add(shardingKeyIndex);
		try {
			dbc = cl.getIndexes();
			while (dbc.hasNext()) {
				BSONObject actual = (BSONObject) dbc.getNext().get("IndexDef");
				actual.removeField("_id");
				if (expect.contains(actual)) {
					expect.remove(actual);
				} else {
					Assert.fail("should not have this index:" + actual);
				}
			}
			Assert.assertEquals(expect.size() == 0, true, "miss some indexes:" + expect);
		} catch (BaseException e) {
			throw e;
		} finally {
			if (dbc != null) {
				dbc.close();
			}
		}
	}

	private void checkGroupData(Sequoiadb db, int expectedCount, String macher, int expectTotalCount,
			String groupName) {
		Sequoiadb destDataNode = null;
		try {
			destDataNode = db.getReplicaGroup(groupName).getMaster().connect();// 获得目标组主节点链接
			DBCollection destCL = destDataNode.getCollectionSpace(csName).getCollection(clName);
			long count = destCL.getCount(macher);

			Assert.assertEquals(count, expectedCount, destDataNode.getServerAddress().toString());// 目标组应当含有上述查询数据
			Assert.assertEquals(destCL.getCount(), expectTotalCount, destDataNode.getServerAddress().toString()); // 目标组应当含有的数据量
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (destDataNode != null) {
				destDataNode.disconnect();
			}
		}
	}

	class Split extends SdbThreadBase {

		@Override
		public void exec() throws Exception {
			Sequoiadb sdb = null;
			try {
				sdb = new Sequoiadb(coordUrl, "", "");
				CollectionSpace cs = sdb.getCollectionSpace(csName);
				DBCollection cl = cs.getCollection(clName);
				cl.split(srcGroupName, destGroupName, 90);
			} catch (BaseException e) {
				System.out.println(srcGroupName);
				System.out.println(destGroupName);
				e.printStackTrace();
				if (sdb.isCollectionSpaceExist(csName)) {
					System.out.println(csName + " exsit");
				} else {
					System.out.println(csName + " not exsit");
				}
				throw e;
			} finally {
				if (sdb != null) {
					sdb.disconnect();
				}
			}
		}

	}

}
