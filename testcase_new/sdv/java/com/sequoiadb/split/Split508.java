package com.sequoiadb.split;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:SEQDB-508 指定多个分区键并发切分1.在CS下创建cl，指定多个分区键，如设置ShardingKey：{"a"：1，"b":-
 *                     1} 2、向cl中插入大量数据，如插入1千万条记录 3、并发执行多个split，设置不同的分区键值进行数据切分
 *                     4、查看数据切分是否正确
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split508 extends SdbTestBase {
	private String clName = "testcaseCL508";
	private String srcGroupName;
	private String destGroupName;
	private Sequoiadb commSdb = null;

	@BeforeClass(enabled = true)
	public void setUp() {
		try {
			commSdb = new Sequoiadb(coordUrl, "", "");

			// 跳过 standAlone 和数据组不足的环境
			CommLib commlib = new CommLib();
			if (commlib.isStandAlone(commSdb)) {
				throw new SkipException("skip StandAlone");
			}
			if (commlib.getDataGroupNames(commSdb).size() < 2) {
				throw new SkipException("current environment less than tow groups ");
			}

			CollectionSpace commCS = commSdb.getCollectionSpace(csName);
			commCS.createCollection(clName,
					(BSONObject) JSON.parse("{ShardingKey:{\"a\":1,\"b\":-1},ShardingType:\"range\"}"));
			ArrayList<String> tmp = SplitUtils.getGroupName(commSdb, csName, clName);
			srcGroupName = tmp.get(0);
			destGroupName = tmp.get(1);
			prepareData(commSdb);// 写入待切分的记录（1000）
		} catch (BaseException e) {
			if (commSdb != null) {
				commSdb.disconnect();
			}
			Assert.fail("Split508 setUp error, error description:" + e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		}
	}

	// 切分范围{a:0,a:20} - {b:20,b:0} 切分范围{a:30,b:50} {b:50,b:30}
	@DataProvider(name = "rangeProvider", parallel = true)
	public Object[][] rangeProvider() {
		return new Object[][] { { 0, 20, 20, 0 }, { 30, 50, 50, 30 } };
	}

	// 切分{a:0,b:20} - {a:20,b:0} 切分{a:30,b:50} {a:50,b:30}
	@Test(enabled = true, dataProvider = "rangeProvider")
	public void splitCL(int aLowBound, int aUpBound, int bLowBound, int bUpBound) {
		Sequoiadb sdb = null;
		try {
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			DBCollection cl = sdb.getCollectionSpace(csName).getCollection(clName);
			cl.split(srcGroupName, destGroupName, (BSONObject) JSON.parse("{a:" + aLowBound + ",b:" + bLowBound + "}"),
					(BSONObject) JSON.parse("{a:" + aUpBound + ",b:" + bUpBound + "}"));

			// 检查目标组切分后数据的正确性
			checkResult(sdb, aLowBound, aUpBound);
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (sdb != null) {
				sdb.disconnect();
			}
		}
	}

	@AfterClass(enabled = true)
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

	// 比对目标组数据正确性
	public void checkResult(Sequoiadb sdb, int alowBound, int aUpBound) {
		Sequoiadb destDataNode = null;
		DBCursor dbc = null;
		try {
			destDataNode = sdb.getReplicaGroup(destGroupName).getMaster().connect();
			DBCollection cl = destDataNode.getCollectionSpace(csName).getCollection(clName);

			// 逐条比对记录
			dbc = cl.query("{a:{$gte:" + alowBound + ",$lte:" + aUpBound + "}}", null, "{a:1}", null);
			int count = alowBound; // 记录key值累加器
			while (dbc.hasNext()) {
				BSONObject actual = dbc.getNext();
				BSONObject expect = (BSONObject) JSON.parse("{a:" + count + ",b:" + count + "}");
				actual.removeField("_id");
				Assert.assertEquals(actual.equals(expect), true, actual.toString() + " " + expect.toString());
				count++;
			}
			Assert.assertEquals(count, alowBound + 21);// 目标组含有本线程切分范围的数据

			long destDataCount1 = destDataNode.getCollectionSpace(csName).getCollection(clName)
					.getCount("{$or:[{a:{$lt:0}},{a:{$gt:20,$lt:30},{a:{$gt:50}}]}");
			Assert.assertEquals(destDataCount1, 0);// 目标组不含切分范围外的数据

		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (dbc != null) {
				dbc.close();
			}
		}
	}

	public void prepareData(Sequoiadb db) {
		try {
			DBCollection cl = db.getCollectionSpace(csName).getCollection(clName);
			ArrayList<BSONObject> arr = new ArrayList<BSONObject>();
			for (int i = 0; i < 1000; i++) {
				arr.add((BSONObject) JSON.parse("{a:" + i + ",b:" + i + "}"));
			}
			cl.bulkInsert(arr, SplitUtils.FLG_INSERT_CONTONDUP);
		} catch (BaseException e) {
			throw e;
		}
	}
}
