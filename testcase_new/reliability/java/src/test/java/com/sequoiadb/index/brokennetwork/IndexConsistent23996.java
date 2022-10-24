package com.sequoiadb.index.brokennetwork;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import com.sequoiadb.base.ReplicaGroup;
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
 * @Description seqDB-23996:复制索引过程中部分子表数据主节点所在主机断网
 * @author wuyan
 * @Date 2021.7.26
 * @version 1.10
 */

public class IndexConsistent23996 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private String groupName1;
    private String groupName2;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String mainclName = "index_maincl_23996";
    private String subclName1 = "index_subcl_23996a";
    private String subclName2 = "index_subcl_23996b";
    private String indexName = "index23996";
    private int recsNum = 50000;
    private long taskId = 0;
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

        List< GroupWrapper > grouplist = groupMgr.getAllDataGroup();
        groupName1 = grouplist.get( 0 ).getGroupName();
        groupName2 = grouplist.get( 1 ).getGroupName();
        System.out.println( "group1:" + groupName1 + " group2:" + groupName2 );

        // create collection and unique index
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( mainclName ) ) {
            cs.dropCollection( mainclName );
        }

        dbcl = createAndAttachCL( cs, mainclName, subclName1, subclName2,
                groupName1, groupName2 );
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    // TODO:http://jira.web:8080/browse/SEQUOIADBMAINSTREAM-8032
    @Test(enabled = false)
    private void test() throws Exception {
        ReplicaGroup cataRG = sdb.getReplicaGroup( "SYSCatalogGroup" );
        System.out.println( "---catalogPrimary hostname ="
                + cataRG.getMaster().getHostName() );
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName1 );
        NodeWrapper masterNode = dataGroup.getMaster();
        FaultMakeTask faultTask = BrokenNetwork.getFaultMakeTask(
                masterNode.hostName(), new java.util.Random().nextInt( 1 ),
                10 );

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

        long[] taskids = { taskId };
        sdb.waitTasks( taskids );
        IndexUtils.checkCopyTask( sdb, SdbTestBase.csName, mainclName,
                indexNames, subclNames );
        int[] resultCodes = new int[] { 0, -247 };
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                subclName1, indexName, resultCodes );
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                subclName2, indexName, resultCodes );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName1,
                indexName, true );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName1,
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
                    System.out.println( "-----cur db = " + db.toString() );
                    System.out
                            .println( "----begin to copy index " + new Date() );
                    taskId = cl.copyIndexAsync( "", indexName );
                    System.out.println( "---end to copy index " + new Date() );
                } catch ( BaseException e ) {
                    System.out.println( "----e---" + e.getErrorType() );
                    if ( e.getErrorType() != SDBError.SDB_CLS_MUTEX_TASK_EXIST
                            .getErrorType()
                            && e.getErrorType() != SDBError.SDB_IXM_REDEF
                                    .getErrorType()
                            && e.getErrorType() != SDBError.SDB_CLS_NOT_PRIMARY
                                    .getErrorType()
                            && e.getErrorType() != SDBError.SDB_NETWORK
                                    .getErrorType() ) {
                        throw e;
                    }
                }
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName1, String subclName2,
            String group1, String group2 ) {
        cs.createCollection( subclName1, ( BSONObject ) JSON
                .parse( "{ShardingKey:{no:1},Group:'" + group1 + "'}" ) );
        cs.createCollection( subclName2, ( BSONObject ) JSON
                .parse( "{ShardingKey:{no:1},Group:'" + group2 + "'}" ) );

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
