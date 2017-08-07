package com.sequoiadb.datasync.killnode;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-3220: 创建多个唯一索引后，写入文档过程中主节点节点异常重启，该主节点为同步的源节点
 * @Author linsuqiang
 * @Date 2017-03-28
 * @Version 1.00
 */

/*
 * 1.创建CS，CL，在CL上创建多个唯一索引 
 * 2.循环执行增删改操作 
 * 3.往副本组中新增节点 
 * 4.过程中购造节点异常重启(kill -9) 
 * 5.选主成功后，继续写入 
 * 6.过程中故障恢复 
 * 7.验证结果
 * 
 * 注：和单独测插入或删除不同，这个用例就是为了覆盖综合的场景
 *    所以特地涉足增删改查和lob操作，没有固定的预期结果，
 *    只要节点间数据一致即可。
 */

public class CRUDWithIndex3220 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clName = "cl_3220";
    private String clGroupName = null;
    private String randomHost = null;
    private int randomPort;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {
            System.out.println("the TestCase Name:" + this.getClass().getName() + ". the TestCase begin at:"
                    + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
            
            groupMgr = new GroupMgr();
            if (!groupMgr.checkBusiness()) {
                throw new SkipException("checkBusiness failed");
            }

            db = new Sequoiadb(coordUrl, "", "");
            clGroupName = groupMgr.getAllDataGroupName().get(0);
            DBCollection cl = createCL(db);
            createIndexes(cl);
            
            // node info, which will be used at AddNodeTask and teardown
            Random ran = new Random();
            List<String> hosts = groupMgr.getAllHosts();
            randomHost = hosts.get(ran.nextInt(hosts.size()));
            randomPort = ran.nextInt(reservedPortEnd - reservedPortBegin) + reservedPortBegin;
            
            Utils.makeReplicaLogFull(clGroupName);
        } catch (ReliabilityException e) {
            Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage() + "\r\n"
                    + Utils.getKeyStack(e, this));
        } finally {
            if (db != null) {
                db.close();
            }
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            GroupWrapper dataGroup = groupMgr.getGroupByName(clGroupName);
            NodeWrapper priNode = dataGroup.getMaster();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(priNode.hostName(), priNode.svcName(), 1);
            TaskMgr mgr = new TaskMgr(faultTask);
            CRUDTask cTask = new CRUDTask();
            AddNodeTask aTask = new AddNodeTask();
            mgr.addTask(cTask);
            mgr.addTask(aTask);
            mgr.execute();
            Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());

            if (!Utils.checkBusinessWithExNode(groupMgr, 600)) { Assert.fail("checkBusiness occurs time out"); }

            db = new Sequoiadb(coordUrl, "", "");
            Utils.testLob(db, clName);
            if (!dataGroup.checkInspect(1)) {
                Assert.fail("data is different on " + dataGroup.getGroupName());
            }
            runSuccess = true;
        } catch (ReliabilityException e) {
            e.printStackTrace();
            Assert.fail(e.getMessage());
        } finally {
            if (db != null) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if (!runSuccess) { throw new SkipException("to save environment"); }
        Sequoiadb db = null;
        try {
            db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            CollectionSpace commCS = db.getCollectionSpace(csName);
            commCS.dropCollection(clName);
            removeNewNode(db);
        } catch (BaseException e) {
            Assert.fail(e.getMessage() + "\r\n" + Utils.getKeyStack(e, this));
        } finally {
            if (db != null) {
                db.close();
            }
            System.out.println("the TestCase Name:" + this.getClass().getName() + ". the TestCase end at:"
                    + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        }
    }
    
    private class CRUDTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb(coordUrl, "", "");
                DBCollection cl = db.getCollectionSpace(SdbTestBase.csName).getCollection(clName);
                int repeatTimes = 5000;
                for (int i = 0; i < repeatTimes; i++) {
                    BSONObject rec = (BSONObject)JSON.parse("{ a: " + i + " }");
                    cl.insert(rec);
                    BSONObject modifier = (BSONObject)JSON.parse("{ $set: { b: 1 } }");
                    cl.update(rec, modifier, null);
                    cl.delete(rec);
                }
            } catch (BaseException e) {
            } finally {
                if (db != null) {
                    db.close();
                }
            }
        }
    }

    
    private class AddNodeTask extends OperateTask {
        @Override
        public void init() {
            // 为了避免节点启动前就已经断网，在启动任务前启动节点
            Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            ReplicaGroup randomGroup = db.getReplicaGroup(clGroupName);
            String nodePath = SdbTestBase.reservedDir + "/data/" + randomPort;
            Node newNode = randomGroup.createNode(randomHost, randomPort, nodePath, (BSONObject)null);
            newNode.start();
            db.close();
        }
        
        @Override
        public void exec() throws Exception {
            // 同步正在后台进行...
        }
    }
    
    private DBCollection createCL(Sequoiadb db) {
        CollectionSpace commCS = db.getCollectionSpace(csName);
        BSONObject option = (BSONObject)JSON.parse("{ Group: '" + clGroupName + "', ReplSize: 1 }");
        return commCS.createCollection(clName, option);
    }
    
    private void createIndexes(DBCollection cl) {
        for (int i = 0; i < 10; i++) {
            String idxName = "idx_" + i;
            BSONObject key = (BSONObject)JSON.parse("{ a" + i + ": 1 }");
            cl.createIndex(idxName, key, true, true, 8);
        }
    }
    
    private void removeNewNode(Sequoiadb db) {
        try {
            GroupWrapper clGroupWrapper = groupMgr.getGroupByName(clGroupName);
            if (clGroupWrapper.getMaster().svcName().equals("" + randomPort)) { 
                clGroupWrapper.changePrimary();
            }
        } catch (ReliabilityException e) {
            e.printStackTrace();
        }
        ReplicaGroup clGroup = db.getReplicaGroup(clGroupName);
        clGroup.removeNode(randomHost, randomPort, (BSONObject)null);
    }
}