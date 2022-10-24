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
 * @Description seqDB-23981 ::创建索引过程中catalog主节点异常
 * @author wuyan
 * @Date 2021.4.23
 * @version 1.10
 */

public class IndexConsistent23981 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String clName = "index_cl_23981";
    private String indexName = "index23981";
    private int recsNum = 100000;
    private long taskId = 0;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
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

    // http://jira.web:8080/browse/SEQUOIADBMAINSTREAM-8733
    @Test(enabled = false)
    private void test() throws Exception {
        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        NodeWrapper priNode = cataGroup.getMaster();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( priNode.hostName(),
                priNode.svcName(), 5 );

        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new CreateIndexTask( indexName ) );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "failed to restore business" );

        // check results
        long[] taskIds = { taskId };
        sdb.waitTasks( taskIds );
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                clName, indexName );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName, true );
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

                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    System.out.println( "----begin to create index" );
                    BasicBSONObject indexKey = new BasicBSONObject();
                    indexKey.put( "testa", 1 );
                    taskId = cl.createIndexAsync( indexName, indexKey, null,
                            null );
                    System.out.println( "---end to create index" );
                } catch ( BaseException e ) {
                    System.out.println( "----e---" + e.getErrorType() );
                    if ( e.getErrorType() != SDBError.SDB_CLS_MUTEX_TASK_EXIST
                            .getErrorType()
                            && e.getErrorType() != SDBError.SDB_CLS_NOT_PRIMARY
                                    .getErrorType() ) {
                        throw e;
                    }
                }
            }
        }
    }
}
