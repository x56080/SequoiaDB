package com.sequoiadb.index.killnode;

import java.util.ArrayList;

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
 * @Description seqDB-23988:取消创建索引任务过程中数据主节点异常
 * @Author liuli
 * @Date 2021.07.31
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.23
 * @version 1.10
 */
public class IndexConsistent23988 extends SdbTestBase {
    private DBCollection dbcl;
    private String groupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String csName = "cs_23988";
    private String clName = "cl_23988";
    private String indexName = "index_23988";
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

        int recsNum = 100000;
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
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

        // check cluster
        Assert.assertTrue( groupMgr.checkBusiness( 600 ),
                "failed to restore business" );

        // check results
        IndexUtils.waitTaskFinish( sdb, csName, clName, "Create index" );

        int[] resultCodes = { 0,
                SDBError.SDB_TASK_HAS_CANCELED.getErrorCode() };
        IndexUtils.checkIndexTask( sdb, "Create index", csName, clName,
                indexName, resultCodes );

        // 可能出现取消任务时任务已经完成，取消任务失败
        int resultCode = getResultCode( sdb, csName + "." + clName );
        IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                resultCode == 0 );
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
        private String indexName;

        private CreateIndexTask( String indexName ) {
            this.indexName = indexName;
        }

        @Override
        public void exec() throws Exception {
            {
                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    // 异步创建索引
                    long taskId = dbcl.createIndexAsync( indexName,
                            new BasicBSONObject( "a", 1 ), null, null );
                    // 随机等待一段时间再删除索引
                    int time = new java.util.Random().nextInt( 2000 );
                    sleep( time );
                    db.cancelTask( taskId, false );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_CLS_NODE_BSFAULT
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_FULL_SYNC
                                    .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_TASK_ALREADY_FINISHED
                                    .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }

    private static int getResultCode( Sequoiadb sdb, String fullName ) {
        DBCursor cursor = sdb.listTasks(
                new BasicBSONObject( "Name", fullName ),
                new BasicBSONObject( "ResultCode", "" ), null, null );
        int resultCode = 0;
        while ( cursor.hasNext() ) {
            resultCode = ( int ) cursor.getNext().get( "ResultCode" );
        }
        Assert.assertFalse( cursor.hasNext() );
        cursor.close();
        return resultCode;
    }
}
