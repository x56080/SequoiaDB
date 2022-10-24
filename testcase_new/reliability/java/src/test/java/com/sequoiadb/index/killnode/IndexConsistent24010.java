package com.sequoiadb.index.killnode;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

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
import com.sequoiadb.commlib.CommLib;
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
 * @Description seqDB-24010 :: 切分表删除索引过程中一个数据组主节点异常
 * @author wuyan
 * @Date 2021.6.22
 * @version 1.10
 */

public class IndexConsistent24010 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private String groupName;
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private CollectionSpace cs = null;
    private String clName = "index_cl_24010";
    private String indexName = "index24010";
    private int recsNum = 100000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "---Skip testCase.Current environment less than tow groups! " );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        List< GroupWrapper > glist = groupMgr.getAllDataGroup();
        srcGroupName = glist.get( 0 ).getGroupName();
        destGroupName = glist.get( 1 ).getGroupName();
        System.out.println(
                "split srcRG:" + srcGroupName + " destRG:" + destGroupName );
        groupName = groupMgr.getAllDataGroup().get( 1 ).getGroupName();
        System.out.println( "cl: " + SdbTestBase.csName + "." + clName
                + ", groupName: " + groupName );

        // create collection and unique index
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        options.put( "Group", srcGroupName );
        dbcl = cs.createCollection( clName, options );

        insertRecords = IndexUtils.insertData( dbcl, recsNum );
        dbcl.split( srcGroupName, destGroupName, 10 );
        dbcl.createIndex( indexName, "{testa:1}", false, false );
    }

    @Test
    private void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( srcGroupName );
        NodeWrapper masterNode = dataGroup.getMaster();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                masterNode.hostName(), masterNode.svcName(), 0 );
        TaskMgr mgr = new TaskMgr( faultTask );

        mgr.addTask( new DeleteIndexTask( indexName ) );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "failed to restore business" );

        // check results
        checkIndex( srcGroupName, false );
        checkIndex( destGroupName, false );
        IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                clName, indexName );
        IndexUtils.checkRecords( dbcl, insertRecords, "", "" );
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
                            .getErrorType() ) {
                        throw e;
                    }
                }
            }
        }
    }

    private void checkIndex( String groupName, Boolean isExistIndex ) {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        for ( NodeWrapper node : dataGroup.getNodes() ) {
            try ( Sequoiadb srcDB = new Sequoiadb(
                    node.hostName() + ":" + node.svcName(), "", "" )) {
                DBCollection dbcl = srcDB
                        .getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                Boolean isExistIndexFlag = dbcl.isIndexExist( indexName );
                Assert.assertEquals( isExistIndexFlag, isExistIndex );
            }
        }
    }
}
