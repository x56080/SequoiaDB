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
 * @description seqDB-24404 :: 删除索引过程中数据节点异常
 * @author wuyan
 * @date 2021.10.15
 * @version 1.10
 */

public class IndexStandalone24404 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private BasicBSONObject clNodeInfo;
    private String nodeName = "";
    private String clName = "index_cl_24404";
    private String indexName = "index24404";
    private int recsNum = 50000;
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
        clNodeInfo = IndexUtils.getCLOneNodeName( sdb, SdbTestBase.csName,
                clName );
        nodeName = clNodeInfo.getString( "hostName" ) + ":"
                + clNodeInfo.getString( "svcName" );
        System.out.println( "---create index node is " + nodeName );
        BSONObject indexKeys = new BasicBSONObject();
        indexKeys.put( "testno", 1 );
        BSONObject indexAttr = new BasicBSONObject();
        indexAttr.put( "Standalone", true );
        BSONObject option = new BasicBSONObject();
        option.put( "NodeName", nodeName );
        dbcl.createIndex( indexName, indexKeys, indexAttr, option );
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    private void test() throws Exception {
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                clNodeInfo.getString( "hostName" ),
                clNodeInfo.getString( "svcName" ), 0 );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new DropIndexTask( indexName ) );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "failed to restore business" );

        // check results
        boolean isExistIndex = isExistIndex( SdbTestBase.csName, clName,
                indexName, nodeName );
        if ( isExistIndex ) {
            IndexUtils.checkStandaloneIndexOnNode( sdb, SdbTestBase.csName,
                    clName, indexName, nodeName, true );
            dbcl.dropIndex( indexName );
            IndexUtils.checkStandaloneIndexOnNode( sdb, SdbTestBase.csName,
                    clName, indexName, nodeName, false );
        } else {
            // 可能出现故障恢复后才执行删除索引
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

    private class DropIndexTask extends OperateTask {
        private String indexName;

        private DropIndexTask( String indexName ) {
            this.indexName = indexName;
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
                    System.out.println( "----begin to drop index" );
                    cl.dropIndex( indexName );
                    System.out.println( "---end to drop index" );
                } catch ( BaseException e ) {
                    System.out.println( "----e---" + e.getErrorType() );
                    if ( e.getErrorType() != SDBError.SDB_COORD_REMOTE_DISC
                            .getErrorType()
                            && e.getErrorType() != SDBError.SDB_CLS_FULL_SYNC
                                    .getErrorType()
                            && e.getErrorType() != SDBError.SDB_IXM_NOTEXIST
                                    .getErrorType()
                            && e.getErrorType() != SDBError.SDB_NET_CANNOT_CONNECT
                                    .getErrorType() ) {
                        throw e;
                    }
                }
            }
        }
    }

    private boolean isExistIndex( String csName, String clName,
            String indexName, String nodeName ) {
        boolean isExistIndex = false;
        try ( Sequoiadb db = new Sequoiadb( nodeName, "", "" )) {
            CollectionSpace cs = db.getCollectionSpace( csName );
            DBCollection cl = cs.getCollection( clName );
            isExistIndex = cl.isIndexExist( indexName );
        }
        return isExistIndex;
    }
}
