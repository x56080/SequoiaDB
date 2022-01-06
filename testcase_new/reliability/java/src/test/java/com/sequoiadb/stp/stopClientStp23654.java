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
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName:seqDB-23654 停止stp client 节点一段时间检查LLT的值和事务操作  
 *           
 * @author chensiqin
 * @version 1.00
 *
 */

public class stopClientStp23654  extends SdbTestBase{

    private boolean clearFlag = true;
    private Sequoiadb sdb;
    private String clName = "testcaseCL23654";
    private CollectionSpace commCS;
    private String node = null;
    private String[] nodeInfo = null;
    private int totalRecord = 100000;
    
    @BeforeClass()
    public void setUp() throws ReliabilityException {
        try {
            StpUtils util = new StpUtils();
            node= util.getStpInfo(SdbTestBase.stpHostName, SdbTestBase.stpServiceName, "getStpClientNode");
            nodeInfo = node.split(",");
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
     * 1.stpstop停止stp client节点一段时间，过程中做事务转账操作，
          * 执行stpq --time检查LLT的正确性和事务操作     
     * */
    
    @Test
    public void test23654() throws InterruptedException {
        try {
            StpUtils util = new StpUtils();
            // 建立并行任务
            TaskMgr mgr = new TaskMgr(  );
            mgr.addTask( new InsertData() );
            mgr.addTask(new stopClientNode());
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
            
            Thread.sleep(10);
            //启动备节点
            String[] tmpNode = nodeInfo[0].split(":");
            util.startStpNode(tmpNode[0]);
          
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
    
    class stopClientNode extends OperateTask {
        @Override
        public void exec() throws Exception {
            try {
                StpUtils util = new StpUtils();
                String[] tmpNode = nodeInfo[0].split(":");
                util.stopStpNode(tmpNode[0]);
            } catch ( BaseException e ) {
                System.out.println(
                        "Attach Thread Exception:" + e.getErrorCode() );
            }
            finally {
               
                if ( sdb != null ) {
                    sdb.close();
                }
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
