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
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-24002:删除索引过程中数据主节点异常
 * @Author liuli
 * @Date 2021.07.28
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.23
 * @version 1.10
 */
public class IndexConsistent24002 extends SdbTestBase {
    private DBCollection dbcl;
    private String groupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String csName = "cs_24002";
    private String clName = "cl_24002";
    private String indexName = "index_24002";
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws Exception {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        groupName = groupMgr.getAllDataGroup().get( 2 ).getGroupName();

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        dbcl = cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );

        int recsNum = 500000;
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
        dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ), false,
                false );
        IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName, true );
    }

    @Test
    private void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper master = dataGroup.getMaster();
        TaskMgr mgr = new TaskMgr();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( master.hostName(),
                master.svcName(), 0 );
        mgr.addTask( faultTask );

        mgr.addTask( new DeleteIndexTask( indexName ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 恢复后校验任务和索引一致性
        Assert.assertTrue( groupMgr.checkBusiness( 600 ),
                "failed to restore business" );

        int[] resultCodes = { 0,
                SDBError.SDB_CLS_NODE_NOT_ENOUGH.getErrorCode(),
                SDBError.SDB_CLS_NOT_PRIMARY.getErrorCode(),
                SDBError.SDB_CLS_FULL_SYNC.getErrorCode(),
                SDBError.SDB_IXM_NOTEXIST.getErrorCode() };
        IndexUtils.checkIndexTask( sdb, "Drop index", csName, clName, indexName,
                resultCodes );

        IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                false );
        IndexUtils.checkRecords( dbcl, insertRecords, "",
                "{'':'" + indexName + "'}" );
    }

    @AfterClass
    private void tearDown() {
        sdb.dropCollectionSpace( csName );
        if ( sdb != null ) {
            sdb.close();
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
                    if ( e.getErrorCode() != SDBError.SDB_INVALID_ROUTEID
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_FULL_SYNC
                                    .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_ENOUGH
                                    .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                                    .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }
}
