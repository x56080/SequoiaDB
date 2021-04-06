package com.sequoiadb.stp;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName: seqDB-23652 备节点多次被强杀后，主节点切到该备节点，检查LLT的值和事务操作 
 *           
 * @author chensiqin
 * @version 1.00
 *
 */

public class KillSpareServer23652  extends SdbTestBase{
    
    private boolean clearFlag = true;
    private Sequoiadb sdb;
    private String clName = "testcaseCL23652";
    private CollectionSpace commCS;
    private int totalRecord = 100000;
    
    @BeforeClass()
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            commCS = sdb.getCollectionSpace( csName );
            if ( commCS.isCollectionExist(clName) ) {
                commCS.dropCollection(clName);
            }
            commCS.createCollection( clName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( this.getClass().getName()
                    + "setUp error, error description:" + e.getMessage() );
        }
    }
    
    /*
     * 1.主节点未同步到备节点，多次强杀该备节点后
     * 2.强杀主节点，切到该备节点
     * 3.过程中做事务转账操作，
         *  执行stpq --time检查LLT的正确性和事务操作 
     *  4.主节点同步到备节点，多次强杀该备节点后，重复步骤23
     * */
    
    @Test
    public void test23652_1() throws InterruptedException {
        try {
            //获取stp 主节点getStpNode
            StpUtils util = new StpUtils();
            //未同步
            String node = util.getStpInfo(SdbTestBase.stpHostName, SdbTestBase.stpServiceName, "getStpSpareNode");
            String[] nodeInfo = node.split(":");
            // 建立并行任务
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    nodeInfo[0], nodeInfo[1], 0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new InsertData() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
            
            node = util.getStpInfo(SdbTestBase.stpHostName, SdbTestBase.stpServiceName, "getStpPrimaryNode");
            nodeInfo = node.split(":");
            FaultMakeTask faultTask2 = KillNode.getFaultMakeTask(
                    nodeInfo[0], nodeInfo[1], 0 );
            TaskMgr mgr2 = new TaskMgr( faultTask2 );
            mgr2.addTask( new InsertData() );
            mgr2.execute();
            Assert.assertEquals( mgr2.isAllSuccess(), true, mgr2.getErrorMsg() );
            
            //查看异常操作后是否切主
            String afterNode = util.getStpInfo(SdbTestBase.stpHostName, SdbTestBase.stpServiceName, "getStpPrimaryNode");
            String primaryHost=afterNode.split(":")[0];
            String primaryPort=afterNode.split(":")[1];
            //校验结果
            String primaryTime1 = util.getStpInfo(primaryHost, primaryPort, "getStpTimeUsAndTimeError");
            String spareStp = util.getStpInfo(SdbTestBase.stpHostName, SdbTestBase.stpServiceName, "getStpSpareNode");
            String spareTime1 = util.getStpInfo(spareStp.split(":")[0], spareStp.split(":")[1], "getStpTimeUsAndTimeError");
            String primaryTime2 = util.getStpInfo(primaryHost, primaryPort, "getStpTimeUsAndTimeError");
            
            util.checkTime(primaryTime1, spareTime1, primaryTime2);
            DBCollection cl = commCS.getCollection(clName);
            Assert.assertEquals(cl.getCount(), totalRecord);
            
        } catch ( ReliabilityException e ) {
            clearFlag = false;
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            sdb.closeAllCursors();
        }
    }
    
    @Test
    public void test23652_2() throws InterruptedException {
        try {
            //获取stp 主节点getStpNode
            StpUtils util = new StpUtils();
            //已同步
            String node = util.getStpInfo(SdbTestBase.stpHostName, SdbTestBase.stpServiceName, "getStpSpareNode");
            String[] nodeInfo = node.split(":");
            // 建立并行任务
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    nodeInfo[0], nodeInfo[1], 0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new InsertData() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
            
            node = util.getStpInfo(SdbTestBase.stpHostName, SdbTestBase.stpServiceName, "getStpPrimaryNode");
            nodeInfo = node.split(":");
            FaultMakeTask faultTask2 = KillNode.getFaultMakeTask(
                    nodeInfo[0], nodeInfo[1], 0 );
            TaskMgr mgr2 = new TaskMgr( faultTask2 );
            mgr2.addTask( new InsertData() );
            mgr2.execute();
            Assert.assertEquals( mgr2.isAllSuccess(), true, mgr2.getErrorMsg() );
            
            //查看异常操作后是否切主
            String afterNode = util.getStpInfo(SdbTestBase.stpHostName, SdbTestBase.stpServiceName, "getStpPrimaryNode");
            //校验结果
            String primaryTime1 = util.getStpInfo(afterNode.split(":")[0], afterNode.split(":")[1], "getStpTimeUsAndTimeError");
            String spareStp = util.getStpInfo(SdbTestBase.stpHostName, SdbTestBase.stpServiceName, "getStpSpareNode");
            String spareTime1 = util.getStpInfo(spareStp.split(":")[0], spareStp.split(":")[1], "getStpTimeUsAndTimeError");
            String primaryTime2 = util.getStpInfo(afterNode.split(":")[0], afterNode.split(":")[1], "getStpTimeUsAndTimeError");
            
            util.checkTime(primaryTime1, spareTime1, primaryTime2);
            DBCollection cl = commCS.getCollection(clName);
            Assert.assertEquals(cl.getCount(), totalRecord);
            
        } catch ( ReliabilityException e ) {
            clearFlag = false;
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            sdb.closeAllCursors();
        }
    }
    
    @AfterClass
    public void tearDown() {
        try {
            if ( clearFlag ) {
                CollectionSpace commCS = sdb.getCollectionSpace( csName );
                commCS.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }

        }
    }
    
    class InsertData extends OperateTask {
        @Override
        public void exec() throws Exception {
            try(Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )){
                DBCollection cl = commCS.getCollection(clName);
                cl.truncate();
                int i = 0;
                sdb.beginTransaction();
                for ( i = 1; i <= totalRecord; i++ ) {
                    BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
                    cl .insert( obj );
                }
                sdb.commit();
            } catch ( BaseException e ) {
                
            }
            finally {
            }
        }
    }
    
}
