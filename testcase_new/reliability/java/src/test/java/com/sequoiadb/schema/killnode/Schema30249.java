package com.sequoiadb.schema.killnode;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.*;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.schema.SchemaUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-30249:外部模式修改字段名时data整组异常
 * @Author liuli
 * @Date 2023.03.09
 * @UpdateAuthor liuli
 * @UpdateDate 2023.03.09
 * @version 1.10
 */
public class Schema30249 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private CollectionSpace dbcs = null;
    private String csName = "cs_30249";
    private String clName = "cl_30249_";
    private String schemaName = "schema_30249_";
    private int schemaNum = 20;
    private String groupName;
    private List< String > schemaNames = new ArrayList<>();

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "StandAlone environment!" );
        }
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        for ( int i = 0; i < schemaNum; i++ ) {
            SchemaUtils.dropSchema( sdb, schemaName + i );
        }

        // 创建 20 个集合并绑定 schema
        groupName = groupMgr.getAllDataGroup().get( 0 ).getGroupName();
        dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32' }, b: { Type: 'int32', WriteDefault: 10 } }" );
        for ( int i = 0; i < schemaNum; i++ ) {
            DBCollection dbcl = dbcs.createCollection( clName + i,
                    new BasicBSONObject( "EnableInfoSchema", true ) );
            sdb.createSchema( schemaName + i, schemaDef );
            dbcl.addSchema( schemaName + i );
        }
    }

    @Test
    public void test() throws Exception {
        // 插入数据
        ArrayList< BSONObject > insertRecords = new ArrayList<>();
        ArrayList< BSONObject > expRecords = new ArrayList<>();
        ArrayList< BSONObject > expPrimalRecords = new ArrayList<>();
        for ( int i = 0; i < 1000; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ) );
            // 构造 add schema 执行成功的数据
            expRecords.add( new BasicBSONObject( "a", i ).append( "c", 10 ) );
            expPrimalRecords
                    .add( new BasicBSONObject( "a", i ).append( "c", 10 ) );
        }
        for ( int i = 0; i < schemaNum; i++ ) {
            DBCollection dbcl = sdb.getCollectionSpace( csName )
                    .getCollection( clName + i );
            dbcl.bulkInsert( insertRecords );
        }

        // add schema 过程中 data 节点整组异常
        TaskMgr mgr = new TaskMgr();
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        System.out.println( "data group -- " + dataGroup.getNodes() );
        for ( NodeWrapper node : dataGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode
                    .getFaultMakeTask( node.hostName(), node.svcName(), 0 );
            mgr.addTask( faultTask );
        }
        for ( int i = 0; i < schemaNum; i++ ) {
            mgr.addTask( new RenameColumn( schemaName + i ) );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待环境恢复
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );

        // 绑定外部模式成功的集合直接校验数据，绑定外部模式失败的集合再次绑定外部模式然后校验数据
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        BasicBSONObject selector = new BasicBSONObject();
        selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
        DBCollection dbcl = null;
        // for ( int i = 0; i < schemaNum; i++ ) {
        // dbcl = sdb.getCollectionSpace( csName ).getCollection( clName + i );
        // if ( !schemaNames.contains( schemaName + i ) ) {
        // // 绑定 schema 后校验数据
        // DBCursor cursor = dbcl.query( null, null, orderBy, null );
        // SchemaUtils.checkRecords( cursor, insertRecords );
        // dbcl.addSchema( schemaName + i );
        // }
        // }

        // 等 LSN 一致后校验主备节点数据一致性
        // Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );
        String oldName = "b";
        String newName = "c";
        String schemaDef = "{ a: { Type: 'int32' }, c: { Type: 'int32', WriteDefault: 10 } }";
        for ( int i = 0; i < schemaNum; i++ ) {
            if ( !schemaNames.contains( schemaName + i ) ) {
                // 绑定 schema 后校验数据
//                dbcl = dbcs.getCollection( clName + i );
//                DBCursor cursor = dbcl.query( null, selector, orderBy, null );
//                SchemaUtils.checkRecords( cursor, insertRecords );
                DBSchema schema = sdb.getSchema( schemaName + i );
                schema.renameColumn( oldName, newName );
            }
            SchemaUtils.checkColumnDef( sdb, schemaName + i, schemaDef );
            SchemaUtils.checkAddSchema( sdb, csName, clName + i,
                    schemaName + i );
            SchemaUtils.checkConsistence( sdb, csName, clName + i, selector,
                    orderBy, expRecords, expPrimalRecords );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            for ( int i = 0; i < schemaNum; i++ ) {
                SchemaUtils.dropSchema( sdb, schemaName + i );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameColumn extends OperateTask {
        private String schemaName;

        private RenameColumn( String schemaName ) {
            this.schemaName = schemaName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待1秒后再绑定 schema
                int time = new java.util.Random().nextInt( 1000 );
                sleep( time );
                DBSchema schema = db.getSchema( schemaName );
                String oldName = "b";
                String newName = "c";
                schema.renameColumn( oldName, newName );
                schemaNames.add( schemaName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK_CLOSE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_NODE_BSFAULT
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_INVALID_ROUTEID
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
