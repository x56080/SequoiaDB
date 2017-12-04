package com.sequoiadb.index;

import java.text.SimpleDateFormat;
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
 * 用例要求： 1、向cl中插入大量数据（如1千万条记录）
 * 2、创建离线方式ID索引，执行创建索引命令createIdIndex({SortBufferSize:16}) 3、创建索引过程中向该CL更新数据
 * 4、查看创建索引结果和数据更新结果
 * 
 * @author huangwenhua
 * @Date 2016.12.14
 * @version 1.00
 */
public class IdIndex6614 extends SdbTestBase {
	private Sequoiadb sdb;
	private SimpleDateFormat df = new SimpleDateFormat(
			"YYYY-MM-dd HH:mm:ss.SSS");
	private CollectionSpace cs;
	private DBCollection cl;
	private String clName = "c6614";

	@BeforeClass
	public void setUp() {
		try {
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		} catch (BaseException e) {
			Assert.fail(" IdIndex6614 setUp error:" + e.getMessage());
		}
		createCL();
		insertData();
	}

	@Test
	public void upsertData() {
		CreateIndex IndexThread = new CreateIndex();
		IndexThread.start();
		Sequoiadb db2 = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		DBCollection cl2 = db2.getCollectionSpace(csName).getCollection(
				this.clName);
		try {
			cl2.update(null, "{$set:{name:\"kkkk\"}}", null);
			if (!IndexThread.isSuccess()) {
				Assert.fail(IndexThread.getErrorMsg());
			}
		} catch (BaseException e) {
			Assert.assertEquals(-279, e.getErrorCode(), e.getMessage());
		} finally {
			if (db2 != null) {
				db2.disconnect();
			}
			IndexThread.join();
		}
	}

	/**
	 * 并发创建索引
	 */
	class CreateIndex extends SdbThreadBase {
		@Override
		public void exec() throws BaseException {

			Sequoiadb db1 = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			try {
				DBCollection cl1 = db1.getCollectionSpace(csName)
						.getCollection(clName);
				BSONObject indexObj = (BSONObject) JSON
						.parse("{SortBufferSize:16}");
				cl1.createIdIndex(indexObj);
				idIndex(cl1);

			} catch (BaseException e) {
				throw e;
			} finally {
				if (db1 != null) {
					db1.disconnect();
				}
			}

		}
	}

	@AfterClass
	public void tearDown() {
		try {
			if (cs.isCollectionExist(clName)) {
				cs.dropCollection(clName);
			}
		} catch (BaseException e) {
			Assert.fail(e.getMessage());
		} finally {
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
					+ "ReplSize:0,Compressed:true,AutoIndexId:false}";
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
			for (int i = 0; i < 1000; i++) {
				BSONObject bson = new BasicBSONObject();
				bson.put("age", i);
				bson.put("name", "Json");
				cl.insert(bson);
			}
		} catch (BaseException e) {
			Assert.fail(" IdIndex6614 insert  error:" + e.getMessage());
		}
	}

	/**
	 * 检查索引
	 */
	public void idIndex(DBCollection cl) {
		DBCursor cursor1 = null;
		try {
			// 通过explain，判断是否走索引
			cursor1 = cl.explain(null, null, null,
					(BSONObject) JSON.parse("{'':'$id'}"), 0, -1, 0, null);
			String scanType = null;
			String indexName = null;
			while (cursor1.hasNext()) {
				BSONObject record = cursor1.getNext();
				if (record.get("Name")
						.equals(SdbTestBase.csName + "." + clName)) {
					scanType = (String) record.get("ScanType");
					indexName = (String) record.get("IndexName");
				}
			}
			Assert.assertEquals(scanType, "ixscan");
			Assert.assertEquals(indexName, "$id");
		} catch (BaseException e) {
			throw e;
		} finally {
			if (cursor1 != null) {
				cursor1.close();
			}
		}
	}

}
