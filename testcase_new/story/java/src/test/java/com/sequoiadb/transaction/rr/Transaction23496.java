package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-23496:开启自动提交，走索引查询
 * @date 2021-1-26
 * @author chenxiaodan
 *
 */

@Test(groups = { "rrauto" })
public class Transaction23496 extends SdbTestBase {
	private Sequoiadb sdb = null;
	private String clName = "cl23496";
	private CollectionSpace cs = null;
	private DBCollection cl = null;
	private List<BSONObject> expList = new ArrayList<>();

	@BeforeClass
	public void setUp() throws InterruptedException {

		sdb = CommLib.getRandomSequoiadb();
		if (CommLib.isStandAlone(sdb)) {
			throw new SkipException("STANDALONE MODE");
		}

		cs = sdb.getCollectionSpace(csName);
		cl = cs.createCollection(clName);
	}

	@Test
	public void test() {
		cl.insert("{_id:1,b:1, a:1}");
		cl.insert("{_id:2,b:2, a:2}");
		cl.insert("{_id:3,b:3, a:3}");
		cl.createIndex("b", "{b:1}", true, false);

		sdb.setSessionAttr((BSONObject) JSON.parse("{TransAutoCommit:true,TransAutoRollback:false}"));
		try {
			cl.update(null, "{$set:{b:10}}", null);
			Assert.fail("update should be failed");
		} catch (BaseException e) {
			Assert.assertEquals(e.getErrorCode(), -38);
		}

		TransUtils.beginTransaction(sdb);
		cl.update("", "{$inc:{a:1}}", null);
		TransUtils.commitTransaction(sdb);
		expList.clear();
		expList = TransUtils.getIncDatas(1, 4, 1);
		TransUtils.queryAndCheck(cl, "{'':null}", expList);
		TransUtils.queryAndCheck(cl, "{'':'b'}", expList);

	}

	@AfterClass
	public void tearDown() {
		cs.dropCollection(clName);
		sdb.close();
	}

}
