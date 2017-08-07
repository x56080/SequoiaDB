package com.sequoiadb.metaopr.diskfull;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.*;

/**
 * @FileName seqDB-2412: 删除CL时catalog主节点所在服务器磁盘满
 * @Author linsuqiang
 * @Date 2017-03-31
 * @Version 1.00
 */

/*
 * 1、创建CS，在该CS下创建多个CL 
 * 2、执行删除CL操作（构造脚本循环执行删除CL操作） 
 * 3、删除CL时catalog主节点所在主机磁盘满（构造主机磁盘满故障） 
 * 3、查看CL信息和catalog主节点状态 
 * 4、恢复故障（清理磁盘空间） 
 * 5、再次执行删除CL操作 
 * 6、查看CL信息（执行listCollections（）命令查看CL信息） 
 * 9、查看catalog主备节点是否存在该CL相关信息
 */

public class DropCL2412 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clNameBase = "cl_2412";
    private static final int CL_NUM = 1000;

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
            CollectionSpace commCS = db.getCollectionSpace(csName);
            for (int i = 0; i < CL_NUM; i++) {
                String clName = clNameBase + "_" + i;
                commCS.createCollection(clName);
            }
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
            GroupWrapper cataGroup = groupMgr.getGroupByName("SYSCatalogGroup");
            NodeWrapper priNode = cataGroup.getMaster();
            Sequoiadb cataDB = priNode.connect();
            DBCollection sysCataCL = cataDB.getCollectionSpace("SYSCAT").getCollection("SYSCOLLECTIONS");
            
            FaultMakeTask faultTask = DiskFull.getFaultMakeTask(priNode.hostName(), SdbTestBase.reservedDir, 0, 10, sysCataCL);
            TaskMgr mgr = new TaskMgr(faultTask);
            mgr.addTask(new DropCLTask());
            mgr.execute();
            Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());
            
            if (!groupMgr.checkBusinessWithLSN(600)) { Assert.fail("checkBusinessWithLSN() occurs timeout"); }
            
            db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            dropCLAgain(db);
            
            if (!groupMgr.checkBusinessWithLSN(600)) { Assert.fail("checkBusinessWithLSN() occurs timeout"); }
            checkListCL(db);
            Utils.checkConsistency(cataGroup);
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
    
    private class DropCLTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb(coordUrl, "", "");
                CollectionSpace commCS = db.getCollectionSpace(csName);
                for (int i = 0; i < CL_NUM; i++) {
                    String clName = clNameBase + "_" + i;
                    commCS.dropCollection(clName);
                }
            } catch (BaseException e) {
            } finally {
                if (db != null) {
                    db.close();
                }
            }
        }
    }
    
    private void dropCLAgain(Sequoiadb db) {
        CollectionSpace commCS = db.getCollectionSpace(csName);
        for (int i = 0; i < CL_NUM; i++) {
            String clName = clNameBase + "_" + i;
            try {
                commCS.dropCollection(clName);
            } catch (BaseException e) {
                // -23 SDB_DMS_NOTEXIST 集合不存在 
                if (e.getErrorCode() != -23) {
                    throw e;
                }
            }
        }
    }
    
    private void checkListCL(Sequoiadb db) {
        // get expect cl name list
        List<BSONObject> expCSNames = new ArrayList<BSONObject>();
        
        // get actual cl name list
        DBCursor cursor = db.listCollections();
        List<BSONObject> actCSNames = new ArrayList<BSONObject>();
        while (cursor.hasNext()) {
            BSONObject result = cursor.getNext();
            actCSNames.add(result);
        }
        cursor.close();
        
        // compare them
        sortByName(actCSNames);
        sortByName(expCSNames);
        if (!actCSNames.equals(expCSNames)) {
            System.out.println(actCSNames);
            System.out.println(expCSNames);
            Assert.fail("listCollections() is not the expected. see details on console");
        }
    }
    
    private void sortByName(List<BSONObject> list) {
        Collections.sort(list, new Comparator<BSONObject>() {
            public int compare(BSONObject a, BSONObject b) {
                String aName = (String)a.get("Name");
                String bName = (String)b.get("Name");
                return aName.compareTo(bName);
            }
        });
    }
}