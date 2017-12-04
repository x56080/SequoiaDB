package com.sequoiadb.split;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.ClientOptions;
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
 *                       删除cl） 4、查看切分和删除cl操作结果 备注此用例验证C
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10528C extends SdbTestBase {
	private String clName = "testcaseCL_10528C";
	private String srcGroupName;
	private String destGroupName;
	private Sequoiadb commSdb = null;
	private AtomicBoolean flag = new AtomicBoolean(false);
	private String customCSName = "testcaseCS_10528C";

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
			CollectionSpace cs = commSdb.createCollectionSpace(customCSName);
			DBCollection cl = cs.createCollection(clName, (BSONObject) JSON
					.parse("{ShardingKey:{'sk':1},ShardingType:'range',Group:'" + srcGroupName + "'}"));
			insertData(cl);// 写入待切分的记录（1000）
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
			for (int i = 0; i < 1000; i++) {
				BSONObject obj = (BSONObject) JSON.parse("{sk:" + i + "}");
				cl.insert(obj);
			}
		} catch (BaseException e) {
			throw e;
		}
	}

	@Test(timeOut = 30 * 60 * 1000)
	public void dropCL() {
		Sequoiadb db = null;
		Sequoiadb dataNode = null;
		Split splitThread = null;
		Random rd = new Random();
		boolean fock = rd.nextBoolean();
		try {

			ClientOptions op = new ClientOptions();
			op.setEnableCache(false);
			Sequoiadb.initClient(op);

			// 切分线程启动
			splitThread = new Split();
			splitThread.start();

			// 等待目标组数据迁移完成
			db = new Sequoiadb(coordUrl, "", "");
			db.setSessionAttr((BSONObject) JSON.parse("{PreferedInstance:'M'}"));
			dataNode = db.getReplicaGroup(destGroupName).getMaster().connect();
			// flag为了防止split线程失败，产生死循环
			while (dataNode.isCollectionSpaceExist(customCSName) != true && flag.get() == false) {
			}
			CollectionSpace cs = dataNode.getCollectionSpace(customCSName);
			try {
				int i = 0;
				while (cs.isCollectionExist(clName) != true && flag.get() == false) {
					// Thread.sleep(100);
					//System.out.println("sleep " + i++);
				}
			} catch (Exception e) {
				e.printStackTrace();
				try {
					dataNode.getCollectionSpace(customCSName);
				} catch (Exception e1) {
					System.out.println("--------flag");
				}
				throw e;
			}
			DBCollection cl = dataNode.getCollectionSpace(customCSName).getCollection(clName);
			while (cl.getCount() != 900 && flag.get() == false) {
			}
			// while (checkCatalog(db) != true && flag.get() != true)
			// ;

			// 删除删除CL,fock是为了随机覆盖：1、数据迁移完成，编目未更新；2、数据迁移完成，编目已更新
			if (fock) {
				Thread.sleep(300);
			}
			db.getCollectionSpace(customCSName).dropCollection(clName);
			Assert.assertEquals(db.getCollectionSpace(customCSName).isCollectionExist(clName), false);

			// 检测切分线程
			Assert.assertEquals(splitThread.isSuccess(), true, splitThread.getErrorMsg());
		} catch (BaseException e) {
			e.printStackTrace();
			Assert.assertEquals(e.getErrorCode(), -147, e.getMessage() + "\r\n" + SplitUtils.getKeyStack(e, this)
					+ " \r\nSplitThread:[" + splitThread.getErrorMsg() + "]  ");
		} catch (InterruptedException e) {
			Assert.fail(e.getMessage() + "\r\n" + SplitUtils.getKeyStack(e, this));
		} finally {
			if (db != null) {
				db.disconnect();
			}
			if (dataNode != null) {
				dataNode.disconnect();
			}
			if (splitThread != null) {
				splitThread.join();
			}
		}
	}

	@AfterClass(enabled = true)
	public void tearDown() {
		try {
			if (commSdb.isCollectionSpaceExist(customCSName)) {
				commSdb.dropCollectionSpace(customCSName);
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
				DBCollection cl = sdb.getCollectionSpace(customCSName).getCollection(clName);
				cl.split(srcGroupName, destGroupName, (BSONObject) JSON.parse("{sk:100}"), // 切分
						(BSONObject) JSON.parse("{sk:1000}"));
			} catch (BaseException e) {
				throw e;
			} finally {
				if (sdb != null) {
					sdb.disconnect();
				}
				flag.set(true);
			}
		}
	}

}
