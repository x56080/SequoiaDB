package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.Date;
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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransRBS;
import com.sequoiadb.transaction.common.TransUtil;

/**
 * @Description seqDB-20523:事务操作过程中，coord节点异常重启
 * @author zhaoyu
 * @date 2020-1-31
 *
 */
@Test(groups = "rr")
public class Transaction20523 extends SdbTestBase {
    private String csName = "cs20523";
    private String clName = "transCL_20523";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private GroupMgr groupMgr;
    private String groupName;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }

        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120 ) ) {
            throw new SkipException( "GROUP ERROR" );
        }
        ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
        groupName = groupNames.get( 0 );
        cl = sdb.createCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        cl.createIndex( "a", "{a:1}", false, false );
        TransRBS.insertDatas( cl );
        TransRBS.genMultiRBSCL( cl );
    }

    @AfterClass
    public void tearDown() throws InterruptedException {
        sdb.commit();
        sdb.dropCollectionSpace( csName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        // 异常重启coord节点
        NodeWrapper coordNode = TransUtil.getCoordNode( sdb );

        // 建立并行任务
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( coordNode,
                new Random().nextInt( 60 ) );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new Update() );
        mgr.addTask( new CreateTask() );
        mgr.execute();

        // TaskMgr检查线程异常
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 最长等待10分钟的集群环境恢复
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ),
                "GROUP ERROR" );

        // 待集群正常后，TR1读记录进行结果校验
        sdb = new Sequoiadb( coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).getCollection( clName );
        DBCursor cursor1 = cl.query( "", "", "{_id:1}", "{'':null}" );
        ArrayList< BSONObject > expDataList = new ArrayList< BSONObject >();
        while ( cursor1.hasNext() ) {
            BSONObject record = cursor1.getNext();
            expDataList.add( record );
        }
        cursor1.close();

        DBCursor cursor2 = cl.query( "", "", "{_id:1}", "{'':'a'}" );
        ArrayList< BSONObject > actDataList = new ArrayList< BSONObject >();
        while ( cursor2.hasNext() ) {
            BSONObject record = cursor2.getNext();
            actDataList.add( record );
        }
        cursor2.close();
        Assert.assertEquals( actDataList, expDataList );
    }

    class Update extends OperateTask {
        Sequoiadb db = null;

        @Override
        public void exec() throws Exception {
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );

                for ( int i = 0; i < 2; i++ ) {
                    System.out.println( "update thread start:" + new Date() );
                    db.beginTransaction();
                    cl.update( null, "{$inc:{a:1}}", "{'':'a'}" );
                    db.commit();
                    System.out.println( "update thread end:" + new Date() );
                }

            } catch ( BaseException e ) {
                e.printStackTrace();
            } finally {
                db.close();
            }

        }
    }

    private class CreateTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace cs = db.getCollectionSpace( csName );
                for ( int i = 0; i < 4000; i++ ) {
                    cs.createCollection( clName + i );
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

}
