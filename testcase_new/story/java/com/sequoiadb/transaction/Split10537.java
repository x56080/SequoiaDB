package com.sequoiadb.transaction;

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
import com.sequoiadb.testcommon.SdbConfTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-10537 切分过程中执行事务操作 1、向cl中插入数据记录 2、执行split，设置切分条件
 *                       3、切分过程中执行事务操作：开启事务，向cl中插入/更新/删除数据（覆盖源组和目标组范围），提交事务
 *                       4、查看切分和事务操作结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10537 extends SdbConfTestBase {
	private String clName = "testcaseCL10537";
	private String srcGroupName;
	private String destGroupName;
	private Sequoiadb commSdb = null;

   @Override
    protected void setNodeConf(){
        dataConf.put("transactionon", true);
        stdalnConf.put("transactionon", true);
    }
    
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

			CollectionSpace customCS = commSdb.getCollectionSpace(csName);
			DBCollection cl = customCS.createCollection(clName, (BSONObject) JSON
					.parse("{ShardingKey:{'sk':1},ShardingType:'range',Group:'group1'}"));
			insertData(cl);// 写入待切分的记录（500）
		} catch (BaseException e) {
			if (commSdb != null) {
				commSdb.disconnect();
			}
			Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage()+"\r\n"+TransUtils.getKeyStack(e,this));
		}
	}

	public void insertData(DBCollection cl) {
		try {
			for (int i = 0; i < 1000; i++) {
				BSONObject obj = (BSONObject) JSON.parse("{sk:" + i + ",alpha:" + i + "}");
				cl.insert(obj);
			}
		} catch (BaseException e) {
			throw e;
		}

	}

	@Test()
	public void transaction() {
		Sequoiadb db = null;
		Split splitThread = null;
		try {
			// 启动切分
			splitThread = new Split();
			splitThread.start();

			// 事务
			db = new Sequoiadb(coordUrl, "", "");
			db.setSessionAttr((BSONObject) JSON.parse("{PreferedInstance:'M'}"));
			db.beginTransaction();
			DBCollection cl = db.getCollectionSpace(csName).getCollection(clName);
			cl.delete("{sk:{$gte:400,$lt:600}}");// 删除数据

			for (int i = 400; i < 600; i++) { // 增加数据
				cl.insert("{sk:" + i + ",beta:1}");
			}
			cl.update("{sk:{$gte:400,$lt:600}}", "{$inc:{beta:1}}", null);// 更新数据
			db.commit();

			// 结果检测
			Assert.assertEquals(splitThread.isSuccess(), true, splitThread.getErrorMsg());
			checkDestAndSrcGroup(db);
			queryUpdateddAndDeltedData(cl);
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+TransUtils.getKeyStack(e,this));
		} finally {
			if (db != null) {
				db.disconnect();
			}
			if (splitThread != null) {
				splitThread.join();
			}
		}
	}

	private void queryUpdateddAndDeltedData(DBCollection cl) {
		DBCursor cusor1 = null;
		try {
			List<BSONObject> expectData = new ArrayList<BSONObject>();
			for (int i = 400; i < 600; i++) {
				expectData.add((BSONObject) JSON.parse("{sk:" + i + ",beta:2}"));
			}

			cusor1 = cl.query("{sk:{$gte:400,$lt:600}}", null, null, null);
			while (cusor1.hasNext()) {
				BSONObject obj = cusor1.getNext();
				obj.removeField("_id");
				if (expectData.contains(obj)) {
					expectData.remove(obj);
				} else {
					Assert.fail("should not find this record:" + obj);
				}
			}
			Assert.assertEquals(expectData.size(), 0, "miss some record:" + expectData);

		} catch (BaseException e) {

		} finally {
			if (cusor1 != null) {
				cusor1.close();
			}
		}

	}

	public void checkDestAndSrcGroup(Sequoiadb db) {
		// 构造源组期望数据
		List<BSONObject> srcExpect = new ArrayList<BSONObject>();
		for (int i = 0; i < 400; i++) {
			srcExpect.add((BSONObject) JSON.parse("{sk:" + i + ",alpha:" + i + "}"));
		}
		for (int i = 400; i < 500; i++) {
			srcExpect.add((BSONObject) JSON.parse("{sk:" + i + ",beta:2}"));
		}
		// 检验源组数据
		checkGroupData(db, srcGroupName, srcExpect);

		// 构造目标组期望数据
		List<BSONObject> destExpect = new ArrayList<BSONObject>();
		for (int i = 500; i < 600; i++) {
			destExpect.add((BSONObject) JSON.parse("{sk:" + i + ",beta:2}"));
		}
		for (int i = 600; i < 1000; i++) {
			destExpect.add((BSONObject) JSON.parse("{sk:" + i + ",alpha:" + i + "}"));
		}
		// 检验目标组数据
		checkGroupData(db, destGroupName, destExpect);
	}

	@AfterClass()
	public void tearDown() {
		try {
			CollectionSpace cs = commSdb.getCollectionSpace(csName);
			cs.dropCollection(clName);
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+TransUtils.getKeyStack(e,this));
		} finally {
			if (commSdb != null) {
				commSdb.disconnect();
			}
		}
	}

	private void checkGroupData(Sequoiadb db, String groupName, List<BSONObject> expect) {
		Sequoiadb dataNode = null;
		DBCursor cursor = null;
		try {
			dataNode = db.getReplicaGroup(groupName).getMaster().connect();// 获得目标组主节点链接
			DBCollection cl = dataNode.getCollectionSpace(csName).getCollection(clName);
			List<BSONObject> actual = new ArrayList<BSONObject>();
			cursor = cl.query(null, null, "{sk:1}", null);
			while (cursor.hasNext()) {
				BSONObject obj = cursor.getNext();
				obj.removeField("_id");
				actual.add(obj);
			}
			Assert.assertEquals(expect.equals(actual), true, "expect:" + expect + "\r\nactual:" + actual);
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+TransUtils.getKeyStack(e,this));
		} finally {
			if (cursor != null) {
				cursor.close();
			}
			if (dataNode != null) {
				dataNode.disconnect();
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
				cl.split(srcGroupName, destGroupName, (BSONObject) JSON.parse("{sk:500}"),
						(BSONObject) JSON.parse("{sk:1000}"));
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
