package com.sequoiadb.index.brokennetwork;

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
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-24007 :: 删除索引过程中数据主节点所在主机断网
 * @author wuyan
 * @Date 2021.4.23
 * @version 1.10
 */

public class IndexConsistent24007 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private String groupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String clName = "index_cl_24007";
    private String indexName = "index24007";
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
        System.out.println( "cl: " + SdbTestBase.csName + "." + clName
                + ", groupName: " + groupName );

        // create collection and unique index
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        dbcl = cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );

        insertRecords = IndexUtils.insertData( dbcl, recsNum );
        dbcl.createIndex( indexName, "{testa:1}", false, false );
    }

    @Test
    private void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper masterNode = dataGroup.getMaster();
        FaultMakeTask faultTask = BrokenNetwork.getFaultMakeTask(
                masterNode.hostName(), new java.util.Random().nextInt( 1 ),
                10 );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new DeleteIndexTask( indexName ) );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "failed to restore business" );

        // check results
        // 0:成功，-47 SDB_IXM_NOTEXIST
        int[] resultCodes = new int[] { 0, -47 };
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                clName, indexName, resultCodes );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName, false );
        runSuccess = true;

    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( clName );
            }
        } finally {
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
                System.out.println( new Date() + " "
                        + this.getClass().getName().toString() );

                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    System.out.println( "----begin to drop index" );
                    cl.dropIndex( indexName );
                    System.out.println( "---end to drop index" );
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
