package com.sequoiadb.split;

import java.text.SimpleDateFormat;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-10528 切分过程中删除CL :1、向cl中循环插入数据记录 2、执行split，设置范围切分条件
 *                       3、切分过程中执行删除CL操作，分别在如下阶段删除CL:
 *                       a、任务已下发还未开始执行（如执行split后，通过listTasks查看无任务，在此过程中删除cl）
 *                       b、迁移数据过程中（如直连目标组节点查看数据持续插入，可count查询数据量在增加，删除cl）
 *                       c、目标组更新编目信息后删除cs（如直连目标组查看数据已迁移完成，或者直连编目节点查看cl信息中存在目标组，
 *                       删除cl） 4、查看切分和删除cl操作结果 备注此用例验证A
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10528A extends SdbTestBase {
	private String clName = "testcaseCL_10528A";
	private String srcGroupName;
	private String destGroupName;
	private Sequoiadb commSdb = null;
	
	
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
					.parse("{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'" + srcGroupName + "'}"));
			insertData(cl);// 写入待切分的记录（10000）
		} catch (BaseException e) {
			if (commSdb != null) {
				commSdb.disconnect();
			}
			Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage() + "\r\n"
					+ SplitUtils.getKeyStack(e, this));
		}
	}

	public void insertData(DBCollection cl) {
		try {
			for (int i = 0; i < 10000; i++) {
				BSONObject obj = (BSONObject) JSON.parse("{sk:" + i + "}");
				cl.insert(obj);
			}
		} catch (BaseException e) {
			throw e;
		}

	}

	@Test
	public void dropCL() {
		Sequoiadb db = null;
		Split splitThread = null;
		try {
			// 切分线程启动
			splitThread = new Split();
			splitThread.start();

			// 删除CL
			db = new Sequoiadb(coordUrl, "", "");
			db.setSessionAttr((BSONObject) JSON.parse("{PreferedInstance:'M'}"));
			CollectionSpace cs = db.getCollectionSpace(csName);
			cs.dropCollection(clName);
			if(cs.isCollectionExist(clName)){
				DBCollection cl = db.getCollectionSpace(csName).getCollection(clName);
				cl.insert("{a:-99}");
			}
			else{
				Assert.assertEquals(db.getCollectionSpace(csName).isCollectionExist(clName), false);
			}
			//Assert.assertEquals(db.getCollectionSpace(csName).isCollectionExist(clName), false);

			// 检测切分结果
			Assert.assertEquals(splitThread.isSuccess(), true, splitThread.getErrorMsg());
		} catch (BaseException e) {
			Assert.assertEquals(e.getErrorCode(), -147, e.getMessage() + "\r\n" + SplitUtils.getKeyStack(e, this)
					+ " \r\nSplitThread:[" + splitThread.getErrorMsg() + "]  ");
		} finally {
			if (db != null) {
				db.disconnect();
			}
			if (splitThread != null) {
				splitThread.join();
			}
		}
	}

	@AfterClass
	public void tearDown() {
		try {
			if (commSdb.getCollectionSpace(csName).isCollectionExist(clName)) {
				commSdb.getCollectionSpace(csName).dropCollection(clName);
			}
		} catch (BaseException e) {
			Assert.fail(e.getMessage() + "\r\n" + SplitUtils.getKeyStack(e, this));
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
				if (cl == null) {// 若cl已被删除，此时cl为空，
					return;
				}
				cl.split(srcGroupName, destGroupName, 90);
				// 如果删除成功，本次测试未碰撞到测试点
			} catch (BaseException e) {
				if (e.getErrorCode() != -23 && e.getErrorCode() != -147) {
					e.printStackTrace();
					throw e;
				}
			} catch (Exception e) {
				e.printStackTrace();
				throw e;
			} finally {
				if (sdb != null) {
					sdb.disconnect();
				}
			}
		}
	}

}
