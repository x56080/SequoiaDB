package com.sequoiadb.index.killnode;

import java.util.ArrayList;
import java.util.Date;

import com.sequoiadb.base.DBCursor;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
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
 * @Description seqDB-23982 :: 创建索引过程中coord节点异常
 * @author wuyan
 * @Date 2021.4.23
 * @version 1.10
 */

public class IndexConsistent23982 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private GroupMgr groupMgr;
    private Sequoiadb testSdb;
    private Sequoiadb sdb;
    private String clName = "index_cl_23982";
    private String indexName = "index23982";
    private int recsNum = 10000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        coordUrl = TransUtil.getCoordUrl( sdb );
        testSdb = new Sequoiadb( coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        // create collection and unique index
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        dbcl = cs.createCollection( clName );

        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    private void test() throws Exception {
        System.out.println(
                "----testsdb=" + testSdb.getHost() + ":" + testSdb.getPort() );
        NodeWrapper coordNode = TransUtil.getCoordNode( testSdb );
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( coordNode, 0 );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new CreateIndexTask( indexName ) );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "failed to restore business" );

        // check results
        boolean isExistTask = IndexUtils.isExistTask( sdb, "Create index",
                SdbTestBase.csName, clName );
        int taskStatus = getTaskStatus( sdb, SdbTestBase.csName, clName,
                "Create index", indexName );
        int finshStatus = 9;
        if ( taskStatus == finshStatus ) {
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    clName, indexName );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                    indexName, true );
        } else {
            // 可能出现coord异常任务没有下发成功，则检查节点都不存在索引
            Assert.assertEquals( taskStatus, 0 );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                    indexName, false );
        }

        dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        IndexUtils.checkRecords( dbcl, insertRecords, "",
                "{'':'" + indexName + "'}" );
        runSuccess = true;

    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CollectionSpace cs = sdb
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
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

                try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    System.out.println( "----begin to create index" );
                    cl.createIndex( indexName, "{testa:1}", false, false );
                    System.out.println( "---end to create index" );
                } catch ( BaseException e ) {
                    System.out.println( "----e---" + e.getErrorType() );
                    if ( e.getErrorType() != SDBError.SDB_NETWORK
                            .getErrorType() ) {
                        throw e;
                    }
                }
            }
        }
    }

    private int getTaskStatus( Sequoiadb db, String csName, String clName,
            String taskTypeDesc, String indexName ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "IndexName", indexName );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        // 如果任务不存在或者任务ready状态未下发到数据节点，则设置状态为0(ready)
        int status = 0;
        BSONObject taskInfo = null;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            status = ( int ) taskInfo.get( "Status" );
            System.out.println( "---getTask=" + taskInfo );
        }
        cursor.close();
        return status;
    }
}
