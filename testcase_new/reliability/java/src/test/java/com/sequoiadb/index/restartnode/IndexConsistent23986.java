package com.sequoiadb.index.restartnode;

import java.util.ArrayList;
import com.sequoiadb.base.ReplicaGroup;
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
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-23986:创建索引过程中数据组切主
 * @Author liuli
 * @Date 2021.07.28
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.23
 * @version 1.10
 */
public class IndexConsistent23986 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private String groupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String csName = "cs_23986";
    private String clName = "cl_23986";
    private String indexName = "index_23986";
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

        // create collection and insert data
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
        // 创建索引过程中，数据组切主
        TaskMgr mgr = new TaskMgr();
        mgr.addTask( new Reelect( groupName ) );
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
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    cl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                            false, false );
                }
            }
        }
    }

    private static class Reelect extends OperateTask {
        private String groupName;

        private Reelect( String groupName ) {
            this.groupName = groupName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                ReplicaGroup rg = db.getReplicaGroup( groupName );
                rg.reelect();
            }
        }
    }
}
