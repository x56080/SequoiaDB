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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;

import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:SEQDB-510 切分范围不冲突，并发切分1.在CS下创建cl，指定分区方式为range
 *                     2、向cl中插入大量数据，如插入1百万条记录
 *                     3、并发执行多个split，其中切分范围不相同，如一个切分范围为(0,10]，另一个切分范围为(80,100]
 *                     4、查看数据切分是否正确
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split510 extends SdbTestBase {
	private String clName = "testcaseCL510";
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
					(BSONObject) JSON.parse("{ShardingKey:{\"a\":1},ShardingType:\"range\"}"));
			ArrayList<String> tmp = SplitUtils.getGroupName(commSdb, csName, clName);
			srcGroupName = tmp.get(0);
			destGroupName = tmp.get(1);

			prepareData(commSdb);// 写入待切分的记录（1000）
		} catch (BaseException e) {
			if (commSdb != null) {
				commSdb.disconnect();
			}
			Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		}
	}

	// 3个切分任务：(a:0,a:100) (a:200,a:300) (a:500,a:600)
	@Test(enabled = true, dataProvider = "rangeProvider")
	public void splitCL(int lowBound, int upBound) {
		Sequoiadb sdb = null;
		try {
			sdb = new Sequoiadb(coordUrl, "", "");
			DBCollection cl = sdb.getCollectionSpace(csName).getCollection(clName);
			cl.split(srcGroupName, destGroupName, (BSONObject) JSON.parse("{a:" + lowBound + "}"),
					(BSONObject) JSON.parse("{a:" + upBound + "}"));

			// 直连目标组主节点，检测目标组切分后数据
			Sequoiadb dataNode = sdb.getReplicaGroup(destGroupName).getMaster().connect();
			DBCollection destGroupCl = dataNode.getCollectionSpace(csName).getCollection(clName);
			long count = destGroupCl.getCount("{a:{$gte:" + lowBound + ",$lt:" + upBound + "}}");

			Assert.assertEquals(count, 100);// 目标组应当含有上述范围数据
			long count1 = destGroupCl.getCount("{a:{$lt:0,$gte:400}}");
			Assert.assertEquals(count1, 0);// 目标组应当不含有含有上述范围数据
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (sdb != null)
				sdb.disconnect();
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

	// 切分范围参数：(a:0,a:100) (a:200,a:300) (a:500,a:600)
	@DataProvider(name = "rangeProvider", parallel = true)
	public Object[][] rangeProvider() {
		return new Object[][] { { 0, 100 }, { 100, 200 }, { 300, 400 } };
	}

	public void prepareData(Sequoiadb db) {
		try {
			DBCollection cl = db.getCollectionSpace(csName).getCollection(clName);
			ArrayList<BSONObject> arr = new ArrayList<BSONObject>();
			for (int i = 0; i < 1000; i++) {
				arr.add((BSONObject) JSON.parse("{a:" + i + "}"));
			}
			cl.bulkInsert(arr, SplitUtils.FLG_INSERT_CONTONDUP);
		} catch (BaseException e) {
			throw e;
		}

	}

}
