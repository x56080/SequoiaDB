package com.sequoiadb.clsmgr.brokennetwork;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName:SEQDB-6201 删除coord节点过程中catalog主节点断网
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class NetDeleteNode6201 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private int coordPort = 26666;
    private String coordDbPath = SdbTestBase.reservedDir;
    private String connectUrl;
    private boolean deleteFlag = false;

    @BeforeClass()
    public void setUp() {
        Sequoiadb sdb = new Sequoiadb(coordUrl, "", "");
        try {
            System.out.println(
                    "the TestCase Name:" + this.getClass().getName() + ". the TestCase begin at:"
                            + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
            groupMgr = new GroupMgr();

            // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
            if (!groupMgr.checkBusiness(20)) {
                throw new SkipException("checkBusiness fail");
            }

        }
        catch (ReliabilityException e) {
            Assert.fail(this.getClass().getName() + " setUp error, error description:"
                    + e.getMessage() + "\r\n" + Utils.getStackString(e));
        }
        finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            GroupWrapper cataGroup = groupMgr.getGroupByName("SYSCatalogGroup");
            String cataPriHost = cataGroup.getMaster().hostName();

            // 得到一个非断网主机的coordurl
            connectUrl = CommLib.getSafeCoordUrl(cataPriHost);

            System.out.println("brokenNetHost:" + cataPriHost + " connectUrl" + connectUrl);

            // 建立一个COORD节点
            db = new Sequoiadb(connectUrl, "", "");
            ReplicaGroup coordGroup = db.getReplicaGroup("SYSCoord");
            Node coordNode = coordGroup.createNode(connectUrl.split(":")[0], coordPort,
                    coordDbPath + "/" + coordPort, new BasicBSONObject());
            coordNode.start();

            // 建立并行任务
            FaultMakeTask faultTask = BrokenNetwork.getFaultMakeTask(cataPriHost, 1, 10, 15);
            TaskMgr mgr = new TaskMgr(faultTask);
            mgr.addTask(new RemoveCoord());
            mgr.execute();
            // TaskMgr检查线程异常
            Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());

            // 最长等待2分钟的集群环境恢复
            Assert.assertEquals(groupMgr.checkBusiness(600), true, "failed to restore business");

            if (!groupMgr.checkResidu()) {
                Sequoiadb tmpDb = null;
                try {
                    tmpDb = new Sequoiadb(connectUrl.split(":")[0] + ":" + coordPort, "", "");
                    coordGroup.removeNode(connectUrl.split(":")[0], coordPort, null);
                }
                finally {
                    if (tmpDb != null) {
                        tmpDb.close();
                    }
                }
            }
        }
        catch (ReliabilityException e) {
            Assert.fail(e.getMessage() + "\r\n" + Utils.getStackString(e));
        }
        finally {
            if (db != null) {
                db.close();
            }
        }

    }

    @AfterClass
    public void tearDown() {
        Sequoiadb sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        try {

        }
        catch (BaseException e) {
            Assert.fail(e.getMessage() + "\r\n" + Utils.getStackString(e));
        }
        finally {
            sdb.close();
            System.out.println(
                    "the TestCase Name:" + this.getClass().getName() + ". the TestCase end at:"
                            + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        }
    }

    class RemoveCoord extends OperateTask {
        @Override
        public void exec() throws Exception {
			  Sequoiadb db = null;
            try {
                db = new Sequoiadb(connectUrl, "", "");
                ReplicaGroup coordGroup = db.getReplicaGroup("SYSCoord");
                coordGroup.removeNode(connectUrl.split(":")[0], coordPort, null);
                deleteFlag = true;
                db.close();
            }
            catch (BaseException e) {
                System.out.println(e.getMessage());
            }finally {
               if(db!=null){
                   db.close();
               }
            }
        }
    }

}
