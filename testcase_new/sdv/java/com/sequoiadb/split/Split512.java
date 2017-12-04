package com.sequoiadb.split;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
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
 * @FileName:SEQDB-512 数据切分过程中插入lob对象 1.在cl下指定分区键进行数据切分
 *                     2、切分过程中向cl中插入大量lob对象，如插入1百万条记录 3、查看数据切分结果
 *                     4、再次插入数据，查看写数据情况
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split512 extends SdbTestBase {
	private String clName = "testcaseCL512";
	private String srcGroupName;
	private String destGroupName;
	Sequoiadb commSdb = null;

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
			commCS.createCollection(clName, (BSONObject) JSON.parse("{ShardingKey:{\"a\":1},ShardingType:\"hash\"}"));
			ArrayList<String> tmp = SplitUtils.getGroupName(commSdb, csName, clName); // 获取切分组名
			srcGroupName = tmp.get(0);
			destGroupName = tmp.get(1);
			prepareData(commSdb);// 准备数据
		} catch (BaseException e) {
			if (commSdb != null) {
				commSdb.disconnect();
			}
			Assert.fail("TestCase512 setUp error, error description:" + e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		}
	}

	// 切分时，插入lob,等待切分完成,检查目标组数据量，重新插入数据，检查落入情况
	@Test(enabled = true)
	public void insertLob() {
		Sequoiadb sdb = null;
		Split split = new Split();
		split.start();
		try {
			sdb = new Sequoiadb(coordUrl, "", "");
			DBCollection cl = sdb.getCollectionSpace(csName).getCollection(clName);
			for (int i = 0; i < 500; i++) {
				DBLob blob = cl.createLob();
				blob.write(clName.getBytes());
				blob.close();
			}
			if(!split.isSuccess()){
				Assert.fail(split.getErrorMsg());
			}
			checkCatalog(sdb);// 检查编目信息
			checkData(sdb); // 检查目标组数据量，重新插入数据，检查落入情况
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (sdb != null) {
				sdb.disconnect();
			}
		}

	}

	public void prepareData(Sequoiadb db) {
		try {
			DBCollection cl = db.getCollectionSpace(csName).getCollection(clName);
			ArrayList<BSONObject> arr = new ArrayList<BSONObject>();
			for (int i = 0; i < 1000; i++) {
				arr.add((BSONObject) JSON.parse("{a:" + i + "}"));
			}
			cl.bulkInsert(arr, 0);

		} catch (BaseException e) {
			throw e;
		}

	}

	// 检查编目信息的切分范围是否正确
	private void checkCatalog(Sequoiadb sdb) {
		DBCursor dbc = null;
		try {
			dbc = sdb.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG, "{Name:\"" + csName + "." + clName + "\"}", null, null);
			BasicBSONList list = null;
			if (dbc.hasNext()) {
				list = (BasicBSONList) dbc.getNext().get("CataInfo");
			} else {
				Assert.fail(clName + " collection catalog not found");
			}
			BSONObject expectLowBound = (BSONObject) JSON.parse("{\"\":2048}");
			BSONObject expectUpBound = (BSONObject) JSON.parse("{\"\":4096}");
			for (int i = 0; i < list.size(); i++) {
				String groupName = (String) ((BSONObject) list.get(i)).get("GroupName");
				if (groupName.equals(destGroupName)) {
					BSONObject actualLowBound = (BSONObject) ((BSONObject) list.get(i)).get("LowBound");
					BSONObject actualUpBound = (BSONObject) ((BSONObject) list.get(i)).get("UpBound");
					if (actualLowBound.equals(expectLowBound) && actualUpBound.equals(expectUpBound)) {
						break;
					} else {
						Assert.fail("check catalog fail");
					}
				}
			}

		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (dbc != null) {
				dbc.close();
			}
		}

	}

	// 检查目标组数据量，重新插入数据，检查落入情况
	private void checkData(Sequoiadb sdb) {
		Sequoiadb destDataNode = null;
		Sequoiadb srcdataNode = null;
		try {
			// 获取目标组主节点链接
			destDataNode = sdb.getReplicaGroup(destGroupName).getMaster().connect();
			// 获取源组主节点链接
			srcdataNode = sdb.getReplicaGroup(srcGroupName).getMaster().connect();

			checkDestGroupDataCount(destDataNode); // 检查目标组数据正确性
			insertAndCheck(sdb, destDataNode, srcdataNode); // 重新插入数据，检查落入情况
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (srcdataNode != null) {
				srcdataNode.disconnect();
			}
			if (destDataNode != null) {
				destDataNode.disconnect();
			}
		}

	}

	// 插入数据，检查落入情况
	public void insertAndCheck(Sequoiadb sdb, Sequoiadb destDataNode, Sequoiadb srcdataNode) {
		DBCursor dbc2 = null;
		DBCursor dbc3 = null;
		try {
			// 插入数据，检查落入情况
			DBCollection cl = sdb.getCollectionSpace(csName).getCollection(clName);
			dbc2 = destDataNode.getCollectionSpace(csName).getCollection(clName).query("", null, null, null, 0, -1);

			// 获取一个目标组的记录，添加一个b字段，去除_id后重新插入,期望此数据落入目标组)
			BSONObject bobj = null;
			if (dbc2.hasNext()) {
				bobj = dbc2.getNext();
			} else {
				Assert.fail("query error");
			}
			bobj.put("b", -10);
			bobj.removeField("_id");
			cl.insert(bobj);

			// 检查是否落入目标组
			DBCollection destGroupCL = destDataNode.getCollectionSpace(csName).getCollection(clName);
			if (!SplitUtils.isCollectionContainThisJSON(destGroupCL, bobj.toString())) {
				Assert.fail("check query data not pass");
			}

			// 获取一个源组的记录，添加一个c字段，去除_id后重新插入,期望此数据落入源组
			dbc3 = srcdataNode.getCollectionSpace(csName).getCollection(clName).query("", null, null, null, 0, -1);
			BSONObject bobj2 = null;
			if (dbc3.hasNext()) {
				bobj2 = dbc3.getNext();
			} else {
				Assert.fail("query error");
			}
			bobj2.put("c", -10);
			bobj2.removeField("_id");
			cl.insert(bobj2);

			// 检查是否落入源组
			DBCollection srcGroupCL = srcdataNode.getCollectionSpace(csName).getCollection(clName);
			if (!SplitUtils.isCollectionContainThisJSON(srcGroupCL, bobj2.toString())) {
				Assert.fail("check query data not pass");
			}
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (dbc2 != null) {
				dbc2.close();
			}
			if (dbc3 != null) {
				dbc3.close();
			}
		}
	}

	// 检查目标组数据量
	private void checkDestGroupDataCount(Sequoiadb destDataNode) {
		DBCursor dbc1 = null;
		try {
			DBCollection cl = destDataNode.getCollectionSpace(csName).getCollection(clName);
			// 统计目标组普通记录数目
			long destDataCount = cl.getCount();

			// 统计目标组Lob数目
			dbc1 = destDataNode.getCollectionSpace(csName).getCollection(clName).listLobs();
			int destLobCount = 0;
			while (dbc1.hasNext()) {
				destLobCount++;
				dbc1.getNext();
			}

			// 检查目标组普通记录数量是否在(500+-(500*0.3))范围内（50%切分，设置出入允许30%）
			if (destDataCount < 500 - (500 * 0.3) || destDataCount > 500 + (500 * 0.3)) {
				Assert.fail("split count unexpeted:"+destDataCount);
			}

			// 检查目标组LOB记录数量是否在(250+-(250*0.3))范围内
			if (destLobCount < 250 - (250 * 0.3) || destLobCount > 250 + (250 * 0.3)) {
				Assert.fail("split count unexpeted:"+destLobCount);
			}
		} catch (BaseException e) {
			Assert.fail(e.getMessage()+"\r\n"+SplitUtils.getKeyStack(e,this));
		} finally {
			if (dbc1 != null) {
				dbc1.close();
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
