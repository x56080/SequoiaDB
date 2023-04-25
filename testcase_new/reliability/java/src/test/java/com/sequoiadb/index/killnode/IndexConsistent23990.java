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
 * @Description seqDB-23990：主表创建索引过程中一个子表所在主节点异常
 * @Author liuli
 * @Date 2021.07.30
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.04
 * @version 1.00
 */
public class IndexConsistent23990 extends SdbTestBase {
    private DBCollection mainCL;
    private String groupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String csName = "cs_23990";
    private String mainCLName = "mainCL_23990";
    private String subCLName1 = "subCL_23990_1";
    private String subCLName2 = "subCL_23990_2";
    private String indexName = "index_23990";
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        groupName = groupMgr.getAllDataGroup().get( 1 ).getGroupName();
        String groupName2 = groupMgr.getAllDataGroup().get( 0 ).getGroupName();

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        // 创建主子表并插入数据
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "IsMainCL", true );
        option.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        option.put( "ShardingType", "range" );
        option.put( "Group", groupName2 );
        mainCL = cs.createCollection( mainCLName, option );

        BasicBSONObject subOption = new BasicBSONObject();
        subOption.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        subOption.put( "ShardingType", "hash" );
        cs.createCollection( subCLName1, subOption );
        subOption.put( "Group", groupName );
        cs.createCollection( subCLName2, subOption );

        BasicBSONObject subCLBound = new BasicBSONObject();
        subCLBound.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        subCLBound.put( "UpBound", new BasicBSONObject( "no", 50000 ) );
        mainCL.attachCollection( csName + "." + subCLName1, subCLBound );

        BasicBSONObject clBound = new BasicBSONObject();
        clBound.put( "LowBound", new BasicBSONObject( "no", 50000 ) );
        clBound.put( "UpBound", new BasicBSONObject( "no", 100000 ) );
        mainCL.attachCollection( csName + "." + subCLName2, clBound );

        int recsNum = 100000;
        insertRecords = IndexUtils.insertData( mainCL, recsNum );
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

        // 恢复后校验任务和索引一致性
        Assert.assertTrue( groupMgr.checkBusiness( 600 ),
                "failed to restore business" );

        int[] resultCodes = { 0, SDBError.SDB_IXM_REDEF.getErrorCode() };
        IndexUtils.checkIndexTask( sdb, "Create index", csName, mainCLName,
                indexName, resultCodes );
        IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName1,
                indexName, resultCodes );
        IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName2,
                indexName, resultCodes );

        Assert.assertTrue( mainCL.isIndexExist( indexName ),
                "Expected index does exist, but actual index not exists" );
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, indexName,
                true );
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, indexName,
                true );
        IndexUtils.checkRecords( mainCL, insertRecords, "",
                "{'':'" + indexName + "'}" );
    }

    @AfterClass
    private void tearDown() {
        sdb.dropCollectionSpace( csName );
        if ( sdb != null ) {
            sdb.close();
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
                            .getCollection( mainCLName );
                    cl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                            false, false );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_INVALID_ROUTEID
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_FULL_SYNC
                                    .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_APP_INTERRUPT
                                    .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }
}
