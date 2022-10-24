package com.sequoiadb.index.killnode;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;

/**
 * @Description seqDB-24297:创建索引过程中coord节点异常，执行取消任务
 * @author wuyan
 * @Date 2021.8.5
 * @version 1.00
 */

public class IndexConsistent24297 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private GroupMgr groupMgr;
    private Sequoiadb testSdb;
    private String getCoordUrl = "";
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String clName = "index_cl_24297";
    private String indexName = "index24297";
    private int recsNum = 100000;
    private boolean isError = false;

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        getCoordUrl = TransUtil.getCoordUrl( sdb );
        System.out.println( "----getCoordUrl=" + getCoordUrl );
        testSdb = new Sequoiadb( getCoordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        // create collection and unique index
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        dbcl = cs.createCollection( clName );
        IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    private void test() throws Exception {
        System.out.println(
                "----testsdb=" + testSdb.getHost() + ":" + testSdb.getPort() );
        NodeWrapper coordNode = TransUtil.getCoordNode( testSdb );
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( coordNode, 3 );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new CreateIndexTask( indexName ) );
        mgr.addTask( new CancelIndexTask( indexName ) );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                "failed to restore business" );

        // check results
        int expResultCode = -243;
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                clName, indexName, expResultCode, false );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName, false );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndexTask extends OperateTask {
        private String indexName;

        private CreateIndexTask( String indexName ) {
            this.indexName = indexName;
        }

        @Override
        public void exec() throws Exception {
            {
                System.out.println( new Date() + " "
                        + this.getClass().getName().toString() );
                try ( Sequoiadb db = new Sequoiadb( getCoordUrl, "", "" )) {
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    System.out.println(
                            "----begin to create index" + new Date() );
                    BasicBSONObject indexKey = new BasicBSONObject();
                    indexKey.put( "testa", 1 );
                    indexKey.put( "no", 1 );
                    BasicBSONObject indexAttr = new BasicBSONObject();
                    indexAttr.put( "Unique", true );
                    cl.createIndex( indexName, indexKey, indexAttr, null );
                    System.out.println( "---end to create index" + new Date() );
                } catch ( BaseException e ) {
                    isError = true;
                    System.out.println( "----e---" + e.getErrorType() );
                    if ( e.getErrorType() != SDBError.SDB_NETWORK.getErrorType()
                            && e.getErrorType() != SDBError.SDB_TASK_HAS_CANCELED
                                    .getErrorType() ) {
                        throw e;
                    }
                }
            }
        }
    }

    private class CancelIndexTask extends OperateTask {
        private String indexName;

        private CancelIndexTask( String indexName ) {
            this.indexName = indexName;
        }

        @Override
        public void exec() throws Exception {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject matcher = new BasicBSONObject();
                matcher.put( "IndexName", indexName );

                int eachSleepTime = 2;
                int maxWaitTime = 3000;
                int alreadyWaitTime = 0;
                boolean isTaskExist = false;
                long taskId = 0;
                do {
                    DBCursor cursorTask = db1.listTasks( matcher, null, null,
                            null );
                    System.out.println( "----listTaks" );
                    while ( cursorTask.hasNext() ) {
                        isTaskExist = true;
                        BSONObject info = cursorTask.getNext();
                        System.out.println( "-----inof=" + info.toString() );
                        taskId = ( long ) info.get( "TaskID" );
                    }
                    cursorTask.close();
                    try {
                        Thread.sleep( eachSleepTime );
                    } catch ( InterruptedException e ) {
                        e.printStackTrace();
                    }
                    alreadyWaitTime += eachSleepTime;
                    if ( alreadyWaitTime > maxWaitTime ) {
                        Assert.fail(
                                "---no task by create index  in maxWaitTime ! waitTime is"
                                        + alreadyWaitTime );
                    }
                } while ( !isTaskExist && !isError );
                System.out.println( "----begin to cancel index" + new Date() );
                db1.cancelTask( taskId, false );
                System.out.println( "---end to cancel index" + new Date() );
            }
        }
    }
}
