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
 * @Description seqDB-23985：强一致要求下创建索引过程中数据备节点异常
 * @Author liuli
 * @Date 2021.08.03
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.03
 * @version 1.00
 */
public class IndexConsistent23985 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private String groupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String csName = "cs_23985";
    private String clName = "cl_23985";
    private String indexName = "index_23985";
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        groupName = groupMgr.getAllDataGroup().get( 1 ).getGroupName();

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        dbcl = cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ).append( "ReplSize",
                        0 ) );

        int recsNum = 100000;
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    private void test() throws Exception {
        // 创建索引过程中数据备节点异常
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper slave = dataGroup.getSlave();
        TaskMgr mgr = new TaskMgr();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( slave.hostName(),
                slave.svcName(), 0 );
        mgr.addTask( faultTask );

        mgr.addTask( new CreateIndexTask( indexName ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 恢复后进行校验
        Assert.assertTrue( groupMgr.checkBusiness( 600 ),
                "failed to restore business" );

        // 校验任务和索引一致性
        IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName, true );
        IndexUtils.checkIndexTaskResult( sdb, "Create index", csName, clName,
                indexName, 0 );
        IndexUtils.checkRecords( dbcl, insertRecords, "",
                "{'':'" + indexName + "'}" );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            sdb.dropCollectionSpace( csName );
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
                    DBCollection dbcl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                            false, false );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_INVALID_ROUTEID
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_FULL_SYNC
                                    .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }
}
