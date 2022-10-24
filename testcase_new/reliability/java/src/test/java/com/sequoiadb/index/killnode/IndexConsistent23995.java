package com.sequoiadb.index.killnode;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
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
import com.sequoiadb.transaction.common.TransUtil;

/**
 * @Description seqDB-23995 :: 复制索引过程中coord节点异常
 * @author wuyan
 * @Date 2021.7.27
 * @version 1.10
 */

public class IndexConsistent23995 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private GroupMgr groupMgr;
    private Sequoiadb testSdb;
    private Sequoiadb sdb;
    private String mainclName = "index_maincl_23995";
    private String subclName1 = "index_subcl_23995a";
    private String subclName2 = "index_subcl_23995b";
    private String indexName = "index23995";
    private int recsNum = 50000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        String useCoordUrl = TransUtil.getCoordUrl( sdb );
        testSdb = new Sequoiadb( useCoordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "---Skip testCase.Current environment less than tow groups! " );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        // create collection and unique index
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( mainclName ) ) {
            cs.dropCollection( mainclName );
        }

        dbcl = createAndAttachCL( cs, mainclName, subclName1, subclName2 );

        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    private void test() throws Exception {
        System.out.println(
                "----testsdb=" + testSdb.getHost() + ":" + testSdb.getPort() );
        NodeWrapper coordNode = TransUtil.getCoordNode( testSdb );
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( coordNode, 1 );

        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new CopyIndexTask( indexName ) );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                "failed to restore business" );

        // check results
        List< String > indexNames = new ArrayList<>();
        indexNames.add( indexName );
        List< String > subclNames = new ArrayList<>();
        subclNames.add( SdbTestBase.csName + "." + subclName1 );
        subclNames.add( SdbTestBase.csName + "." + subclName2 );
        IndexUtils.waitTaskFinish( sdb, SdbTestBase.csName, mainclName,
                "Copy index" );
        IndexUtils.checkCopyTask( sdb, SdbTestBase.csName, mainclName,
                indexNames, subclNames );
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                subclName1, indexName );
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                subclName2, indexName );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName1,
                indexName, true );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName2,
                indexName, true );
        IndexUtils.checkRecords( dbcl, insertRecords, "",
                "{'':'" + indexName + "'}" );
        runSuccess = true;

    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CollectionSpace cs = sdb
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( mainclName );
            }
        } finally {
            sdb.close();
            if ( testSdb != null ) {
                testSdb.close();
            }
        }
    }

    private class CopyIndexTask extends OperateTask {
        private String indexName;

        private CopyIndexTask( String indexName ) {
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
                            .getCollection( mainclName );
                    System.out.println( "----begin to copy index" );
                    cl.copyIndex( "", indexName );
                    System.out.println( "---end to copy index" );
                } catch ( BaseException e ) {
                    System.out.println( "----e---" + e.getErrorType() );
                    if ( e.getErrorType() != SDBError.SDB_NETWORK
                            .getErrorType() ) {
                        throw e;
                    }
                }
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName1, String subclName2 ) {
        cs.createCollection( subclName1,
                ( BSONObject ) JSON.parse( "{ShardingKey:{no:1}}" ) );
        cs.createCollection( subclName2 );

        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainclName, optionsM );

        mainCL.createIndex( indexName, "{testa:1}", false, false );

        mainCL.attachCollection( csName + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:10000}}" ) );
        mainCL.attachCollection( csName + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:10000},UpBound:{no:50000}}" ) );
        return mainCL;
    }
}
