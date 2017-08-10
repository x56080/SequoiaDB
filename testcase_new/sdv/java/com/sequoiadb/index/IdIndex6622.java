package com.sequoiadb.index;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * 用例要求： 1、指定已存在id索引的cl，指定索引查询 2、查询过程中删除集合中的 $id 索引（dropIdIndex（）） 3、查看返回的结果是否正确
 * 
 * @author huangwenhua
 * @Date 2016.12.14
 * @version 1.00
 */
public class IdIndex6622 extends SdbTestBase {
	private Sequoiadb sdb;
	private SimpleDateFormat df = new SimpleDateFormat(
			"YYYY-MM-dd HH:mm:ss.SSS");
	private CollectionSpace cs;
	private DBCollection cl;
	private String clName = "c6622";
	private ArrayList<BSONObject> insertData = null;

	@BeforeClass
	public void setUp() {
		try {
			System.out.println("the TestCase: " + this.getClass().getName()
					+ " begin at:" + df.format(new Date()));
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		} catch (BaseException e) {
			Assert.fail(" IdIndex6622 setUp error:" + e.getMessage());
		}
		createCL();
		insertData();
	}

	@Test
	public void queryData() {
		Sequoiadb db2 = null;
		DBCursor cursor = null;
		BSONObject actRecs = null;
		DropIdIndex DropIdIndexThread = new DropIdIndex();
		DropIdIndexThread.start();
		try {
			db2 = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			DBCollection cl2 = db2.getCollectionSpace(csName).getCollection(
					this.clName);
			// 查询一定要遍历
			cursor = cl2.query(null, null, null, "{'':'$id'}");
			while (cursor.hasNext()) {
				actRecs = cursor.getNext();
			}
			if (!DropIdIndexThread.isSuccess()) {
				Assert.fail(DropIdIndexThread.getErrorMsg());
			}
			Assert.fail("dropIdIndex not begin" + actRecs);
		} catch (BaseException e) {
			Assert.assertEquals(-48, e.getErrorCode(), e.getMessage());

		} finally {
			if (cursor != null) {
				cursor.close();
			}
			if (db2 != null) {
				db2.disconnect();
			}
		}
	}

	class DropIdIndex extends SdbThreadBase {
		@Override
		public void exec() throws Exception {
			DBCursor cursor1 = null;
			Sequoiadb db3 = null;
			try {
				db3 = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				DBCollection cl3 = db3.getCollectionSpace(csName)
						.getCollection(clName);
				cl3.dropIdIndex();
				// 通过explain，是否走索引,判断索引是否删除成功
				cursor1 = cl3.explain(null, null, null,
						(BSONObject) JSON.parse("{'':'$id'}"), 0, -1, 0, null);
				String scanType = null;
				while (cursor1.hasNext()) {
					BSONObject record = cursor1.getNext();
					if (record.get("Name").equals(
							SdbTestBase.csName + "." + clName)) {
						scanType = (String) record.get("ScanType");
					}
				}
				Assert.assertEquals(scanType, "tbscan");
			} catch (BaseException e) {
				throw e;
			} finally {
				cursor1.close();
				if (db3 != null) {
					db3.disconnect();
				}
			}
		}
	}

	@AfterClass
	public void tearDown() {
		try {
			if (this.cs.isCollectionExist(clName)) {
				this.cs.dropCollection(clName);
			}
		} catch (BaseException e) {
			Assert.fail(e.getMessage());
		} finally {
			System.out.println("the TestCase Name:" + this.getClass().getName()
					+ ". the TestCase end at:" + this.df.format(new Date()));
			if (sdb != null) {
				sdb.disconnect();
			}
		}
	}

	public void createCL() {
		try {
			if (!sdb.isCollectionSpaceExist(SdbTestBase.csName)) {
				sdb.createCollectionSpace(SdbTestBase.csName);
			}
		} catch (BaseException e) {
			// -33 CS exist,ignore exceptions
			Assert.assertEquals(-33, e.getErrorCode(), e.getMessage());
		}
		try {
			String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
					+ "ReplSize:0,Compressed:true}";
			BSONObject options = (BSONObject) JSON.parse(clOptions);
			cs = sdb.getCollectionSpace(SdbTestBase.csName);
			cl = cs.createCollection(clName, options);
		} catch (BaseException e) {
			Assert.assertTrue(false, "create cl fail " + e.getErrorType() + ":"
					+ e.getMessage());
		}
	}

	public void insertData() {
		try {
			insertData = new ArrayList<BSONObject>();
			for (int i = 0; i < 5000; i++) {
				BSONObject bson = new BasicBSONObject();
				bson.put("age", i);
				bson.put("name", "Json" + i);
				this.cl.insert(bson);
				insertData.add(bson);
			}
			cl.bulkInsert(insertData, DBCollection.FLG_INSERT_CONTONDUP);
		} catch (BaseException e) {
			Assert.fail(" IdIndex6622 insert error:" + e.getMessage());
		}
	}

}
