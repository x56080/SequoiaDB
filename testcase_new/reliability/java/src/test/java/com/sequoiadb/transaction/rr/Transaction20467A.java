package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
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
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransRBS;

/**
 * @Description seqDB-20467:事务操作过程中，catalog整组异常重启
 * @author zhaoyu
 * @date 2020-1-31
 *
 */
@Test(groups = "rr")
public class Transaction20467A extends SdbTestBase {
    private String csName = "cs20467A";
    private String clName = "transCL_20467A";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private GroupMgr groupMgr;

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
        cl = sdb.createCollectionSpace( csName ).createCollection( clName );
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
        sdb.beginTransaction();

        // 获取集合中的记录作为预期结果
        DBCursor cursor = cl.query( "", "", "{_id:1}", "{'':null}" );
        ArrayList< BSONObject > expDataList = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            expDataList.add( record );
        }
        cursor.close();

        // 异常重启整组catalog节点
        TaskMgr mgr = new TaskMgr();
        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        List< String > cataAllUrls = cataGroup.getAllUrls();
        for ( int i = 0; i < cataAllUrls.size(); i++ ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    cataAllUrls.get( i ).split( ":" )[ 0 ],
                    cataAllUrls.get( i ).split( ":" )[ 1 ], 60 );
            mgr.addTask( faultTask );
        }

        // 建立并行任务
        mgr.addTask( new Update() );
        mgr.addTask( new CreateTask() );
        mgr.execute();

        // TaskMgr检查线程异常
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 最长等待10分钟的集群环境恢复
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ),
                "GROUP ERROR" );

        // 待集群正常后，TR1读记录进行结果校验
        cl = sdb.getCollectionSpace( csName ).getCollection( clName );
        DBCursor cursor1 = cl.query( "", "", "{_id:1}", "{'':null}" );
        ArrayList< BSONObject > actDataList = new ArrayList< BSONObject >();
        while ( cursor1.hasNext() ) {
            BSONObject record = cursor1.getNext();
            actDataList.add( record );
        }
        cursor1.close();
        Assert.assertEquals( actDataList, expDataList );

        actDataList.clear();
        DBCursor cursor2 = cl.query( "", "", "{_id:1}", "{'':'a'}" );
        while ( cursor2.hasNext() ) {
            BSONObject record = cursor2.getNext();
            actDataList.add( record );
        }
        cursor2.close();
        Assert.assertEquals( actDataList, expDataList );

        // 校验catalog节点创建集合正常
        sdb.getCollectionSpace( csName )
                .createCollection( clName + "_testCatalog" );

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
                throw e;
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
