package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-23507:混合隔离级别，RR事务在更新后开启，查询符合RR隔离级别
 * @date 2021-01-27
 * @author chenxiaodan
 */
@Test(groups = { "rr" })
public class Transaction23507 extends SdbTestBase {
	private Sequoiadb db1 = null;
	private Sequoiadb db2 = null;
	private String clName = "cl23507";
	private DBCollection cl1 = null;
	private DBCollection cl2 = null;
	private List<BSONObject> expList = new ArrayList<>();

	@BeforeClass
	public void setUp() throws InterruptedException {
		db1 = CommLib.getRandomSequoiadb();
		db2 = CommLib.getRandomSequoiadb();
		if (CommLib.isStandAlone(db1)) {
			throw new SkipException("STANDALONE MODE");
		}
		cl1 = db1.getCollectionSpace(csName).createCollection(clName);

		BSONObject record1 = (BSONObject) JSON.parse("{_id:1,a:1, b:1}");
		expList.add(record1);
		BSONObject record2 = (BSONObject) JSON.parse("{_id:2,a:2, b:2}");
		expList.add(record2);

		cl1.createIndex("a", "{a:1}", true, false);
		cl1.insert(record1);
		cl1.insert(record2);

	}

	@Test
	public void test() throws InterruptedException {
		// 连接2，开启rc写事务，执行操作
		db2.setSessionAttr((BSONObject) JSON.parse("{TransIsolation: 1}"));
		TransUtils.beginTransaction(db2);
		cl2 = db2.getCollectionSpace(csName).getCollection(clName);

		cl2.insert("{_id:3, a:3, b:3}");
		cl2.update("{_id:1}", "{$set:{a:4}}", null);
		cl2.delete("{_id:2}");

		// 连接1，开启rr读事务，查询结果
		db1.setSessionAttr((BSONObject) JSON.parse("{TransIsolation: 3}"));
		TransUtils.beginTransaction(db1);

		TransUtils.queryAndCheck(cl1, "{'':null}", expList);
		TransUtils.queryAndCheck(cl1, "{'':'a'}", expList);

		// 连接2提交事务
		TransUtils.commitTransaction(db2);

		// 连接1查询结果
		TransUtils.queryAndCheck(cl1, "{'':null}", expList);
		TransUtils.queryAndCheck(cl1, "{'':'a'}", expList);
		TransUtils.commitTransaction(db1);

	}

	@AfterClass
	public void tearDown() {
		db1.getCollectionSpace(csName).dropCollection(clName);
		db1.close();
		db2.close();
	}

}
