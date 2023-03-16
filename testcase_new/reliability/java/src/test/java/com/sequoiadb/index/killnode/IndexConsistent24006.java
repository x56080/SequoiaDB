package com.sequoiadb.index.killnode;

import java.util.ArrayList;

import com.sequoiadb.commlib.CommLib;
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
 * @Description seqDB-24006:删除索引过程中coord节点异常
 * @Author liuli
 * @Date 2021.07.29
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.23
 * @version 1.10
 */
public class IndexConsistent24006 extends SdbTestBase {
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private Sequoiadb sdb2;
    private String csName = "cs_24005";
    private String clName = "cl_24005";
    private String indexName = "index_24005";
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();
    private String safeCoordUrl;

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        safeCoordUrl = CommLib.getSafeCoordUrl( sdb.getHost() );
        if ( safeCoordUrl == null ) {
            throw new SkipException(
                    "skip datasource cluster less than tow coords" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        String groupName = groupMgr.getAllDataGroup().get( 1 ).getGroupName();

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        DBCollection dbcl = cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );

        int recsNum = 100000;
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
        dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ), false,
                false );
    }

    @Test
    private void test() throws Exception {
        sdb2 = new Sequoiadb( safeCoordUrl, "", "" );
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( sdb2.getHost(),
                String.valueOf( sdb2.getPort() ), 0 );
        TaskMgr mgr = new TaskMgr();
        mgr.addTask( faultTask );

        mgr.addTask( new DeleteIndexTask( indexName, safeCoordUrl ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 恢复后校验任务和索引一致性
        Assert.assertTrue( groupMgr.checkBusiness( 600 ),
                "failed to restore business" );

        IndexUtils.checkIndexTask( sdb, "Drop index", csName, clName, indexName,
                0 );
        IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                false );
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
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
            if ( sdb2 != null ) {
                sdb2.close();
            }
        }
    }

    private class DeleteIndexTask extends OperateTask {
        private String indexName;
        private String CoordUrl;

        private DeleteIndexTask( String indexName, String CoordUrl ) {
            this.indexName = indexName;
            this.CoordUrl = CoordUrl;
        }

        @Override
        public void exec() throws Exception {
            {
                try ( Sequoiadb db = new Sequoiadb( CoordUrl, "", "" )) {
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    cl.dropIndex( indexName );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_NETWORK
                            .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }
}
