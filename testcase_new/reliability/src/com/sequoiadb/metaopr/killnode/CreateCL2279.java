package com.sequoiadb.metaopr.killnode;

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
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.*;

/**
 * @FileName seqDB-2279: 创建CL时catalog备节点异常重启（不指定Domain）
 * @Author linsuqiang
 * @Date 2017-03-31
 * @Version 1.00
 */

/*
 * 1、创建CS，在该CS下创建CL（执行脚本构造循环执行创建多个CL操作） 
 * 2、创建CL时catalog备节点异常重启（如执行kill -9杀掉节点进程，构造节点异常重启） 
 * 3、查看CL创建结果和catalog备节点状态 
 * 4、节点启动成功后（查看节点进程存在） 
 * 5、再次创建相同CL，向该CL中插入数据 
 * 6、查看CL信息（执行db.listCollections（）命令查看CS/CL信息是否和实际一致 
 * 7、查看catalog主备节点是否存在该CL相关信息
 */

public class CreateCL2279 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clNameBase = "cl_2279";
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
            NodeWrapper slvNode = cataGroup.getSlave();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(slvNode.hostName(), slvNode.svcName(), 0);
            TaskMgr mgr = new TaskMgr(faultTask);
            CreateCLTask cTask = new CreateCLTask();
            mgr.addTask(cTask);
            mgr.execute();
            Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());
            
            if (!groupMgr.checkBusinessWithLSN(600)) { Assert.fail("checkBusinessWithLSN() occurs timeout"); }
            
            db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            createCLAgain(db);
            operateOnCL(db);

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
            db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            dropCL(db);
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
    
    private class CreateCLTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb(coordUrl, "", "");
                CollectionSpace commCS = db.getCollectionSpace(csName);
                for (int i = 0; i < CL_NUM; i++) {
                    String clName = clNameBase + "_" + i;
                    commCS.createCollection(clName);
                }
            } catch (BaseException e) {
            } finally {
                if (db != null) {
                    db.close();
                }
            }
        }
    }
    
    private void createCLAgain(Sequoiadb db) {
        CollectionSpace commCS = db.getCollectionSpace(csName);
        for (int i = 0; i < CL_NUM; i++) {
            String clName = clNameBase + "_" + i;
            try {
                commCS.createCollection(clName);
            } catch (BaseException e) {
                // -22 SDB_DMS_EXIST 集合已存在 
                if (e.getErrorCode() != -22) {
                    throw e;
                }
            }
        }
    }
    
    private void operateOnCL(Sequoiadb db) {
        CollectionSpace commCS = db.getCollectionSpace(csName);
        for (int i = 0; i < CL_NUM; i++) {
            String clName = clNameBase + "_" + i;
            DBCollection cl = commCS.getCollection(clName);
            cl.insert("{ a: 1 }");
        }
    }
    
    private void checkListCL(Sequoiadb db) {
        // get expect cl name list
        List<BSONObject> expCSNames = new ArrayList<BSONObject>();
        for (int i = 0; i < CL_NUM; i++) {
            BSONObject nameBSON = new BasicBSONObject();
            String clFullName = csName + "." + clNameBase + "_" + i;
            nameBSON.put("Name", clFullName);
            expCSNames.add(nameBSON);
        }
        
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
    
    private void dropCL(Sequoiadb db) {
        CollectionSpace commCS = db.getCollectionSpace(csName);
        for (int i = 0; i < CL_NUM; i++) {
            String clName = clNameBase + "_" + i;
            commCS.dropCollection(clName);
        }
    }
}