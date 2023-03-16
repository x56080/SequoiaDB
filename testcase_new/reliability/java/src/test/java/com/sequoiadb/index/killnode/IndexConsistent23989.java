package com.sequoiadb.index.killnode;

import java.util.ArrayList;
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
 * @Description seqDB-23989:切分表创建索引过程中一个数据组主节点异常
 * @Author liuli
 * @Date 2021.07.28
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.23
 * @version 1.10
 */
public class IndexConsistent23989 extends SdbTestBase {
    private DBCollection dbcl;
    private String groupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String csName = "cs_23989";
    private String clName = "cl_23989";
    private String indexName = "index_23989";
    private int recsNum = 100000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        groupName = groupMgr.getAllDataGroup().get( 1 ).getGroupName();
        String groupName2 = groupMgr.getAllDataGroup().get( 0 ).getGroupName();

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        dbcl = cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ).append( "ShardingKey",
                        new BasicBSONObject( "no", 1 ) ) );

        insertRecords = IndexUtils.insertData( dbcl, recsNum );
        dbcl.split( groupName, groupName2, 50 );
    }

    @Test
    private void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper master = dataGroup.getMaster();
        TaskMgr mgr = new TaskMgr();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( master.hostName(),
                master.svcName(), 0 );
        mgr.addTask( faultTask );

        mgr.addTask( new CreateIndexTask( indexName ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 恢复后校验任务和索引一致性
        Assert.assertTrue( groupMgr.checkBusiness( 600 ),
                "failed to restore business" );
        int[] resultCodes = { 0, SDBError.SDB_TASK_HAS_CANCELED.getErrorCode(),
                SDBError.SDB_INVALID_ROUTEID.getErrorCode(),
                SDBError.SDB_CLS_FULL_SYNC.getErrorCode(),
                SDBError.SDB_IXM_REDEF.getErrorCode() };
        IndexUtils.checkIndexTask( sdb, "Create index", csName, clName,
                indexName, resultCodes );

        IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName, true );
        IndexUtils.checkRecords( dbcl, insertRecords, "",
                "{'':'" + indexName + "'}" );
    }

    @AfterClass
    private void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class CreateIndexTask extends OperateTask {
        private final String indexName;

        private CreateIndexTask( String indexName ) {
            this.indexName = indexName;
        }

        @Override
        public void exec() throws Exception {
            {
                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    cl.createIndex( indexName,
                            new BasicBSONObject( "a", 1 ).append( "no", 1 ),
                            false, false );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_INVALID_ROUTEID
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_FULL_SYNC
                                    .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                                    .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_COORD_NODE_CAT_VER_OLD
                                    .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }
}
