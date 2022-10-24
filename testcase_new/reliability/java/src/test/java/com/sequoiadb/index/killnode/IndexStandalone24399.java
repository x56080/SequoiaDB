package com.sequoiadb.index.killnode;

import java.util.ArrayList;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
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

/**
 * @description seqDB-24399 :: 创建索引过程中数据节点异常
 * @author wuyan
 * @date 2021.4.23
 * @version 1.10
 */

public class IndexStandalone24399 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String clName = "index_cl_24399";
    private String indexName = "index24399";
    private int recsNum = 50000;
    private boolean isCreateIndexOK = false;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        dbcl = cs.createCollection( clName,
                new BasicBSONObject( "ReplSize", 0 ) );

        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    private void test() throws Exception {
        BasicBSONObject nodeInfo = IndexUtils.getCLOneNodeName( sdb,
                SdbTestBase.csName, clName );
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                nodeInfo.getString( "hostName" ),
                nodeInfo.getString( "svcName" ), 3 );

        String nodeName = nodeInfo.getString( "hostName" ) + ":"
                + nodeInfo.getString( "svcName" );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new CreateIndexTask( indexName, nodeName ) );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "failed to restore business" );

        // check results
        if ( !isCreateIndexOK ) {
            IndexUtils.checkStandaloneIndexOnNode( sdb, SdbTestBase.csName,
                    clName, indexName, nodeName, false );
            // 再次创建相同索引
            createIndexStandAlone( dbcl, indexName, nodeName );
            IndexUtils.checkStandaloneIndexOnNode( sdb, SdbTestBase.csName,
                    clName, indexName, nodeName, true );
        } else {
            // 可能未触发故障时创建索引，则创建本地索引成功，节点异常重启同步后回滚删除索引
            IndexUtils.checkStandaloneIndexOnNode( sdb, SdbTestBase.csName,
                    clName, indexName, nodeName, false );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            try {
                CollectionSpace cs = sdb
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( clName );
            } finally {
                if ( sdb != null ) {
                    sdb.close();
                }
            }
        }
    }

    private class CreateIndexTask extends OperateTask {
        private String indexName;
        private String nodeName;

        private CreateIndexTask( String indexName, String nodeName ) {
            this.indexName = indexName;
            this.nodeName = nodeName;
        }

        @Override
        public void exec() throws Exception {
            {
                System.out.println( new Date() + " "
                        + this.getClass().getName().toString() );

                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    createIndexStandAlone( cl, indexName, nodeName );
                    isCreateIndexOK = true;
                } catch ( BaseException e ) {
                    System.out.println( "----e---" + e.getErrorType() );
                    if ( e.getErrorType() != SDBError.SDB_COORD_REMOTE_DISC
                            .getErrorType()
                            && e.getErrorType() != SDBError.SDB_CLS_FULL_SYNC
                                    .getErrorType() ) {
                        throw e;
                    }
                }
            }
        }
    }

    private void createIndexStandAlone( DBCollection cl, String indexName,
            String nodeName ) {
        BSONObject indexKeys = new BasicBSONObject();
        indexKeys.put( "testno", 1 );
        BSONObject indexAttr = new BasicBSONObject();
        indexAttr.put( "Standalone", true );
        BSONObject option = new BasicBSONObject();
        option.put( "NodeName", nodeName );
        cl.createIndex( indexName, indexKeys, indexAttr, option );
    }
}
