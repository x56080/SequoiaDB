package com.sequoiadb.index.diskfull;

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
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-23983:创建索引过程中数据主节点所在主机磁盘满
 * @Author liuli
 * @Date 2021.08.10
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.10
 * @version 1.00
 */
public class IndexConsistent23983 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private String groupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String csName = "cs_23983";
    private String clName = "cl_23983";
    private String indexName = "index_23983";
    private int recsNum = 100000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws Exception {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        groupName = groupMgr.getAllDataGroup().get( 2 ).getGroupName();
        System.out.println(
                "cl: " + csName + "." + clName + ", groupName: " + groupName );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        dbcl = cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );

        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    private void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper master = dataGroup.getMaster();
        TaskMgr mgr = new TaskMgr();
        FaultMakeTask faultTask = DiskFull.getFaultMakeTask( master.hostName(),
                master.dbPath(), 0, 10 );
        mgr.addTask( faultTask );

        mgr.addTask( new CreateIndexTask( indexName ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 恢复后校验任务和索引一致性
        Assert.assertTrue( groupMgr.checkBusiness( 600 ),
                "failed to restore business" );

        IndexUtils.checkIndexTask( sdb, "Create index", csName, clName,
                indexName, 0 );
        IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                runSuccess );
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
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    System.out.println( "----begin to create index" );
                    cl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                            false, false );
                    System.out.println( "---end to create index" );
                } catch ( BaseException e ) {
                    System.out.println( "----e---" + e.getErrorType() );
                    if ( e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                            .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }
}
