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
 * @FileName:seqDB-23651已刷盘时server组主节点多次被强杀检查LLT的值和事务操作
 *           
 * @author chensiqin
 * @version 1.00
 *
 */

public class KillPrimaryServer23651  extends SdbTestBase{
    
    private boolean clearFlag = true;
    private Sequoiadb sdb;
    private String clName = "testcaseCL23651";
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
     * 1.server主节点已刷盘（刷盘时间默认60s）的情况下，多次强杀stp主节点导致主节点切主，
         * 通过stpq --time检查LLT的正确性，过程中同时做转账操作 
     * 2.server主节点已刷盘（刷盘时间默认60s）的情况下，多次强杀stp主节点（未切主），
         * 通过stpq --time检查LLT的正确性，过程中同时做转账操作     
     * */
    
    @Test
    public void test23651() throws InterruptedException {
        try {
            //获取stp 主节点getStpNode
            StpUtils util = new StpUtils();
            String node = util.getStpInfo(SdbTestBase.stpHostName, SdbTestBase.stpServiceName, "getStpPrimaryNode");
            String[] nodeInfo = node.split(":");
            // 建立并行任务
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    nodeInfo[0], nodeInfo[1], 0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new InsertData() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
            
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
