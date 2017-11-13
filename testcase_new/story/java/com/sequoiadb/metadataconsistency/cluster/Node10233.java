package com.sequoiadb.metadataconsistency.cluster;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.AfterClass;
import org.testng.Assert;
import org.testng.SkipException;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadataconsistency.data.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10233:concurrency[createNode, removeNode]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.25
 */

public class Node10233 extends SdbTestBase {
	private SimpleDateFormat dateFm = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss");
	private static Sequoiadb sdb = null;
	private String rgName = "rg10233";
	private Random random = new Random();
	private int msec = 1000;

	@BeforeClass
	public void setUp() {
		// start time
		System.out
				.println("Begin to run " + getClass().getName() + ", begin in: " + dateFm.format(new Date().getTime()));
		try {
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			// judge the mode and group number
			if (CommLib.isStandAlone(sdb) || CommLib.OneGroupMode(sdb)) {
				throw new SkipException("The mode is standlone, or only one group, skip the testCase.");
			}
			CommLib.clearGroup(sdb, rgName);

			sdb.createReplicaGroup(rgName);
			CommLib.createNode(sdb, rgName, SdbTestBase.reservedPortBegin, SdbTestBase.reservedPortEnd,
					SdbTestBase.reservedDir);
			ReplicaGroup rgDB = sdb.getReplicaGroup(rgName);
			rgDB.start();
		} catch (BaseException e) {
			sdb.disconnect();
			Assert.fail(e.getMessage());
		}

	}

	@AfterClass
	public void tearDown() {
		try {
			CommLib.clearGroup(sdb, rgName);
		} catch (BaseException e) {
			Assert.fail(e.getMessage());
		} finally {
			System.out
					.println("End to run " + getClass().getName() + ", end in: " + dateFm.format(new Date().getTime()));
			sdb.disconnect();
		}
	}

	@Test
	public void test() {
		CreateNode createNode = new CreateNode();
		createNode.start();

		RemoveNode removeNode = new RemoveNode();
		CommLib.sleep(random.nextInt(msec));
		removeNode.start();

		if (!(createNode.isSuccess() && removeNode.isSuccess())) {
			Assert.fail(createNode.getErrorMsg() + removeNode.getErrorMsg());
		}

		// check results
		CommLib.checkRGOfCatalog(rgName);
	}

	private class CreateNode extends SdbThreadBase {
		@Override
		public void exec() throws BaseException {
			Sequoiadb db = null;
			try {
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");

				CommLib.createNode(db, rgName, SdbTestBase.reservedPortBegin, SdbTestBase.reservedPortEnd,
						SdbTestBase.reservedDir);
				ReplicaGroup rgDB = db.getReplicaGroup(rgName);
				rgDB.start();
			} catch (BaseException e) {
				int eCode = e.getErrorCode();
				if (eCode != -156) {
					throw e;
				}
			} finally {
				db.disconnect();
			}
		}
	}

	private class RemoveNode extends SdbThreadBase {
		@Override
		public void exec() throws BaseException {
			Sequoiadb db = null;
			try {
				db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
				ReplicaGroup rgDB = db.getReplicaGroup(rgName);

				Node node = rgDB.getSlave();
				String hostName = node.getHostName();
				int svcName = node.getPort();
				rgDB.removeNode(hostName, svcName, null);
			} catch (BaseException e) {
				int eCode = e.getErrorCode();
				if (eCode != -155) {
					throw e;
				}
			} finally {
				db.disconnect();
			}
		}
	}

}