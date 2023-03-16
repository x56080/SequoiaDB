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
 * @Description seqDB-24005:删除索引过程中catalog主节点异常
 * @Author liuli
 * @Date 2021.07.29
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.23
 * @version 1.10
 */
public class IndexConsistent24005 extends SdbTestBase {
    private DBCollection dbcl;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String csName = "cs_24005";
    private String clName = "cl_24005";
    private String indexName = "index_24005";
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        String groupName = groupMgr.getAllDataGroup().get( 1 ).getGroupName();

        // create collection and unique index
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        dbcl = cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );

        int recsNum = 100000;
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
        dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ), false,
                false );
    }

    @Test
    private void test() throws Exception {
        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        NodeWrapper priNode = cataGroup.getMaster();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( priNode.hostName(),
                priNode.svcName(), 0 );
        TaskMgr mgr = new TaskMgr();
        mgr.addTask( faultTask );

        mgr.addTask( new DeleteIndexTask( indexName ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 恢复后校验任务和索引一致性
        Assert.assertTrue( groupMgr.checkBusiness( 600 ),
                "failed to restore business" );

        IndexUtils.checkIndexTask( sdb, "Drop index", csName, clName, indexName,
                0 );
        IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                false );
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

    private class DeleteIndexTask extends OperateTask {
        private String indexName;

        private DeleteIndexTask( String indexName ) {
            this.indexName = indexName;
        }

        @Override
        public void exec() throws Exception {
            {
                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    cl.dropIndex( indexName );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_CLS_MUTEX_TASK_EXIST
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_INVALID_ROUTEID
                                    .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }
}
