package com.sequoiadb.index;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

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
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * 用例要求： 1、向cl中插入大量数据（如1千万条记录）
 * 2、创建排序方式ID索引，执行创建索引命令createIdIndex({SortBufferSize:32}) 3、创建索引过程中向该CL插入数据
 * 4、查看创建索引结果和插入数据情况
 * 
 * @author huangwenhua
 * @Date 2016.12.14
 * @version 1.00
 */
public class IdIndex6613 extends SdbTestBase {
	private Sequoiadb sdb;
	private SimpleDateFormat df = new SimpleDateFormat(
			"YYYY-MM-dd HH:mm:ss.SSS");
	private CollectionSpace cs;
	private DBCollection cl;
	private String clName = "c6613";
	private ArrayList<BSONObject> insertRecods;

	@BeforeClass
	public void setUp() {
		try {
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		} catch (BaseException e) {
			Assert.fail("TestIndex6613 setUp error, error description:"
					+ e.getMessage());
		}
		createCL();
	}

	@Test
	public void insertData() {

		CreateIndex IndexThread = new CreateIndex();
		IndexThread.start();
		try {
			this.insertRecods = new ArrayList<BSONObject>();
			BSONObject bson;
			for (int i = 0; i < 1000; i++) {
				bson = new BasicBSONObject();
				bson.put("_id", i);
				bson.put("age", i);
				this.insertRecods.add(bson);
			}
			cl.bulkInsert(this.insertRecods, DBCollection.FLG_INSERT_CONTONDUP);
			if (!IndexThread.isSuccess()) {
				Assert.fail(IndexThread.getErrorMsg());
			}
		} catch (BaseException e) {
			Assert.fail("Index6613 insert error:" + e.getMessage());
		} finally {
			IndexThread.join();
			// 检查插入要确保索引已建
			checkInsert(cl);
		}
	}

	/**
	 * 创建索引
	 */
	class CreateIndex extends SdbThreadBase {
		@Override
		public void exec() throws BaseException {
			Sequoiadb sdb1 = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			try {
				DBCollection cl1 = sdb1.getCollectionSpace(SdbTestBase.csName)
						.getCollection(clName);
				BSONObject indexObj2 = (BSONObject) JSON
						.parse("{SortBufferSize:32}");
				cl1.createIdIndex(indexObj2);
				chekIndex(cl1);

			} catch (BaseException e) {
				throw e;
			} finally {
				if (sdb1 != null) {
					sdb1.disconnect();
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
			if (!this.sdb.isCollectionSpaceExist(SdbTestBase.csName)) {
				this.sdb.createCollectionSpace(SdbTestBase.csName);
			}
		} catch (BaseException e) {
			// -33 CS 集合空间已存在
			Assert.assertEquals(-33, e.getErrorCode(), e.getMessage());
		}
		try {
			String clOptions = "{ShardingKey:{a:1},ShardingType:'hash',Partition:1024,"
					+ "ReplSize:0,Compressed:true,AutoIndexId:false}";
			BSONObject options = (BSONObject) JSON.parse(clOptions);
			cs = sdb.getCollectionSpace(SdbTestBase.csName);
			cl = cs.createCollection(this.clName, options);
		} catch (BaseException e) {
			Assert.assertTrue(false, "create cl fail " + e.getErrorType() + ":"
					+ e.getMessage());
		}
	}

	/**
	 * 检查插入
	 */
	public void checkInsert(DBCollection cl) {
		DBCursor cursor = null;
		try {
			cursor = cl.query(null, null, "{_id:1}", "{'':'$id'}");
			List<BSONObject> actual = new ArrayList<BSONObject>();
			while (cursor.hasNext()) {
				BSONObject obj = cursor.getNext();
				actual.add(obj);
			}
			// 对比插入值和查询值
			Assert.assertEquals(actual, this.insertRecods);
		} catch (BaseException e) {
			Assert.fail("IdIndex6613 insert error:" + e.getMessage());
		} finally {
			if (cursor != null) {
				cursor.close();
			}
		}
	}

	/**
	 * 检查索引
	 */
	public void chekIndex(DBCollection cl) {
		DBCursor cursor1 = null;
		try {
			// 通过explain，判断是否走索引
			cursor1 = cl.explain(null, null, null,
					(BSONObject) JSON.parse("{'':'$id'}"), 0, -1,
					DBQuery.FLG_QUERY_FORCE_HINT, null);
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
