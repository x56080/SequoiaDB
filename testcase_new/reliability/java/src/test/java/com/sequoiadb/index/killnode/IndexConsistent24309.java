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
import com.sequoiadb.base.DBCursor;
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
 * @Description seqDB-24309:删除索引过程中数据主节点异常，取消任务
 * @Author liuli
 * @Date 2021.08.03
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.03
 * @version 1.00
 */
public class IndexConsistent24309 extends SdbTestBase {
    private DBCollection dbcl;
    private String groupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String csName = "cs_24309";
    private String clName = "cl_24309";
    private String indexName = "index_24309";
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
                new BasicBSONObject( "Group", groupName ).append( "ReplSize",
                        0 ) );

        int recsNum = 100000;
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
        dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ), false,
                false );
    }

    @Test
    private void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper master = dataGroup.getMaster();
        TaskMgr mgr = new TaskMgr();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( master.hostName(),
                master.svcName(), 0, 5 );
        mgr.addTask( faultTask );

        mgr.addTask( new DropIndexTask( indexName ) );
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
        sdb.dropCollectionSpace( csName );
        if ( sdb != null ) {
            sdb.close();
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
                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    cl.dropIndex( indexName );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_TASK_HAS_CANCELED
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_FULL_SYNC
                                    .getErrorCode() ) {
                        throw e;
                    }
                }
                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    BasicBSONObject matcher = new BasicBSONObject();
                    matcher.put( "Name", csName + "." + clName );
                    matcher.put( "IndexName", indexName );
                    matcher.put( "TaskTypeDesc", "Drop index" );
                    DBCursor taskStatus = db.listTasks( matcher, null, null,
                            null );
                    while ( taskStatus.hasNext() ) {
                        BSONObject task = taskStatus.getNext();
                        String statusDesc = task.get( "StatusDesc" ).toString();
                        if ( !"Finish".equals( statusDesc )
                                && !"End".equals( statusDesc ) ) {
                            long taskId = ( long ) task.get( "TaskID" );
                            db.cancelTask( taskId, false );
                        }
                    }
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_CLS_NODE_BSFAULT
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_TASK_ALREADY_FINISHED
                                    .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }
}
