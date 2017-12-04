package com.sequoiadb.split;

import java.io.UnsupportedEncodingException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

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
 * @FileName:SEQDB-10530 切分过程中插入数据 :1、向cl中插入数据记录 2、执行split，设置切分条件
 *                       3、切分过程中插入数据（记录+lob），持续插入数据覆盖如下阶段: a、迁移数据过程中（插入数据包含迁移数据）
 *                       b、清除数据过程中（插入数据包含清除数据） 插入数据同时满足如下条件： a、包含切分范围边界值数据
 *                       b、覆盖源组和目标组范围 4、查看切分和插入操作结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10530 extends SdbTestBase {
	private String clName = "testcaseCL_10530";
	private String srcGroupName;
	private String destGroupName;
	private Sequoiadb commSdb = null;
	List<BSONObject> insertedData = new ArrayList<BSONObject>();// 所有已插入的数据
	List<ObjectId> insertedLob = new ArrayList<ObjectId>(); // 所有已插入的lobid

	@BeforeClass()
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

			CollectionSpace commCS = commSdb.getCollectionSpace(csName);
			DBCollection cl = commCS.createCollection(clName,
					(BSONObject) JSON
							.parse("{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
									+ srcGroupName + "'}"));
			insertData(cl);// 写入待切分的记录（500）
		} catch (BaseException e) {
			if (commSdb != null) {
				commSdb.disconnect();
			}
			Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		}
	}

	public void insertData(DBCollection cl) {
		try {
			for (int i = 0; i < 500; i++) {
				BSONObject obj = (BSONObject) JSON.parse("{sk:" + i + "}");
				insertedData.add(obj);
				cl.insert(obj);
			}
		} catch (BaseException e) {
			throw e;
		}
	}

	// 切分同时插入数据，检查
	@Test
	public void insertAndCheck() {
		Sequoiadb db = null;
		Split splitThread = new Split();
		splitThread.start();
		try {
			db = new Sequoiadb(coordUrl, "", "");

			// 插入数据和Lob
			insertDataAndLob(db);

			if (!splitThread.isSuccess()) {
				Assert.fail(splitThread.getErrorMsg());
			}
			
			// 校验源和目标组普通记录
			ArrayList<BSONObject> insertedDataCopy = new ArrayList<BSONObject>(insertedData);
			// 目标组中的数据应当是insertDataCopy的子集，校验完成后，将删除insertDataCopy中属于子集的元素
			checkGroupData(insertedDataCopy, db, destGroupName);
			// 源中的数据应当是insertDataCopy的子集，校验完成后，将删除insertDataCopy中属于子集的元素
			checkGroupData(insertedDataCopy, db, srcGroupName);
			// 经过两次校验，insertDataCopy应当为空
			Assert.assertEquals(insertedDataCopy.size() == 0, true,
					"srcGroup and destGroup can not find:" + insertedDataCopy);

			// 校验源和目标组LOB记录
			ArrayList<ObjectId> insertedLobCopy = new ArrayList<ObjectId>(insertedLob);
			checkGroupLob(insertedLobCopy, db, destGroupName);
			checkGroupLob(insertedLobCopy, db, srcGroupName);
			Assert.assertEquals(insertedLobCopy.size() == 0, true,
					"srcGroup and destGroup can not find:" + insertedLobCopy);

			checkCoord(db);
		} catch (BaseException e) {
			
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (db != null) {
				db.disconnect();
			}
		}
	}

	private void checkCoord(Sequoiadb db) {
		DBCursor cursor1 = null;
		DBCursor cursor2 = null;
		try {
			db.setSessionAttr((BSONObject) JSON.parse("{PreferedInstance:'M'}"));
			DBCollection commCL = db.getCollectionSpace(csName).getCollection(clName);

			// 将insertedData中的每一条数据作为macher(覆盖查询边界)
			for (int i = 0; i < insertedData.size(); i++) {
				cursor1 = commCL.query(insertedData.get(i), null, null, null);
				BSONObject expect = insertedData.get(i);
				if (cursor1.hasNext()) {
					BSONObject actual = cursor1.getNext();
					Assert.assertEquals(expect.equals(actual), true, "expect:" + expect + " actual:" + actual);
					if (cursor1.hasNext()) {
						Assert.fail("query more than tow record mach expetedData:" + cursor1.getNext());
					}
				} else {
					Assert.fail("query can not find:" + expect);
				}
			}

			// 将insertedLob中的每一个ID作为macher(覆盖查询边界)
			for (int j = 0; j < insertedLob.size(); j++) {
				DBLob lob = commCL.openLob(insertedLob.get(0));
				byte[] buffer = new byte[128];
				int length = lob.read(buffer);
				String content = new String(buffer, 0, length, "UTF-8");
				Assert.assertEquals(lob.getID().toString().equals(content), true,
						"expect:" + lob.getID() + " actual:" + content);
				lob.close();
			}

		} catch (BaseException | UnsupportedEncodingException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (cursor1 != null) {
				cursor1.close();
			}
			if (cursor2 != null) {
				cursor2.close();
			}
		}
	}

	private void checkGroupLob(List<ObjectId> insertedLob, Sequoiadb sdb, String destGroupName) {
		Sequoiadb destDataNode = null;
		DBCursor cursor = null;
		try {
			destDataNode = sdb.getReplicaGroup(destGroupName).getMaster().connect();// 获得源主节点链接
			DBCollection destCL = destDataNode.getCollectionSpace(csName).getCollection(clName);

			cursor = destCL.listLobs();
			int lobCount = 0;
			while (cursor.hasNext()) {
				ObjectId oid = (ObjectId) cursor.getNext().get("Oid");
				DBLob lob = destCL.openLob(oid);
				Assert.assertEquals(insertedLob.contains(lob.getID()), true);
				byte[] buffer = new byte[128];
				int length = lob.read(buffer);
				String content = new String(buffer, 0, length, "UTF-8");
				Assert.assertEquals(lob.getID().toString().equals(content), true);
				lob.close();
				lobCount++;
				insertedLob.remove(lob.getID());
			}
			// 数据量应在250条左右（总量500，切分范围2048-4096）
			Assert.assertEquals(lobCount > 250 - (250 * 0.3) && lobCount < 250 + (250 * 0.3), true,
					"srcGroup count:" + lobCount);
		} catch (BaseException | UnsupportedEncodingException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (cursor != null) {
				cursor.close();
			}
			if (destDataNode != null) {
				destDataNode.disconnect();
			}
		}
	}

	private void checkGroupData(List<BSONObject> insertedData, Sequoiadb sdb, String groupName) {
		Sequoiadb dataNode = null;
		DBCursor cursor = null;
		try {
			dataNode = sdb.getReplicaGroup(groupName).getMaster().connect();// 获得目标组主节点链接
			DBCollection cl = dataNode.getCollectionSpace(csName).getCollection(clName);

			cursor = cl.query(null, null, "{sk:1}", null);
			while (cursor.hasNext()) {
				BSONObject actual = cursor.getNext();
				Assert.assertEquals(insertedData.contains(actual), true,
						"insertedData can not find this record:" + actual);
				insertedData.remove(actual);
			}
			long count = cl.getCount();
			// 组的数据量应该在500条左右（总量500+500，切分范围2048-4096）
			Assert.assertEquals(count > 500 - (500 * 0.3) && count < 500 + (500 * 0.3), true,
					"destGroup data count:" + count);
		} catch (BaseException e) {
			e.printStackTrace();
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (cursor != null) {
				cursor.close();
			}
			if (dataNode != null) {
				dataNode.disconnect();
			}
		}
	}

	private void insertDataAndLob(Sequoiadb db) {
		try {
			DBCollection cl = db.getCollectionSpace(csName).getCollection(clName);
			for (int i = 500; i < 1000; i++) {
				BSONObject obj = (BSONObject) JSON.parse("{sk:" + i + "}");
				cl.insert(obj);
				insertedData.add(obj);

				DBLob lob = cl.createLob();
				String id = lob.getID().toString();
				lob.write(id.getBytes());
				lob.close();
				insertedLob.add(lob.getID());
			}
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		}
	}

	@AfterClass()
	public void tearDown() {
		try {
			CollectionSpace commCS = commSdb.getCollectionSpace(csName);

			commCS.dropCollection(clName);

		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
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
				DBCollection cl = sdb.getCollectionSpace(csName).getCollection(clName);
				cl.split(srcGroupName, destGroupName, 50);
			} catch (BaseException e) {
				throw e;
			} finally {
				if (sdb != null) {
					sdb.disconnect();
				}
			}
		}

	}

}
