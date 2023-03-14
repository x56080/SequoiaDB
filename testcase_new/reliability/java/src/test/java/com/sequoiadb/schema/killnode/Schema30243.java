package com.sequoiadb.schema.killnode;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.*;
import com.sequoiadb.schema.SchemaUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-30243:绑定外部模式过程中catalog主节点异常
 * @Author liuli
 * @Date 2023.03.08
 * @UpdateAuthor liuli
 * @UpdateDate 2023.03.08
 * @version 1.10
 */
public class Schema30243 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private CollectionSpace dbcs = null;
    private String csName = "cs_30243";
    private String clName = "cl_30243_";
    private String schemaName = "schema_30243_";
    private BasicBSONObject schemaDef = new BasicBSONObject();
    private int schemaNum = 20;
    private List< String > schemaNames = new ArrayList<>();
    private List< String > clNames = new ArrayList<>();

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

        dbcs = sdb.createCollectionSpace( csName );
        // 创建 20 个集合由于绑定外部模式
        schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32' }, b: { Type: 'int32', ReadDefault: 10 } }" );
        for ( int i = 0; i < schemaNum; i++ ) {
            dbcs.createCollection( clName + i,
                    new BasicBSONObject( "EnableInfoSchema", true ) );
            sdb.createSchema( schemaName + i, schemaDef );
        }
    }

    @Test
    public void test() throws Exception {
        // 插入数据
        ArrayList< BSONObject > insertRecords = new ArrayList<>();
        ArrayList< BSONObject > expRecords = new ArrayList<>();
        for ( int i = 0; i < 1000; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ) );
            // 构造 add schema 执行成功的数据
            expRecords.add( new BasicBSONObject( "a", i ).append( "b", 10 ) );
        }
        for ( int i = 0; i < schemaNum; i++ ) {
            DBCollection dbcl = dbcs.getCollection( clName + i );
            dbcl.bulkInsert( insertRecords );
        }

        // 绑定 schema 过程中 catalog 主节点异常
        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        NodeWrapper priNode = cataGroup.getMaster();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( priNode.hostName(),
                priNode.svcName(), 0 );

        TaskMgr mgr = new TaskMgr( faultTask );
        for ( int i = 0; i < schemaNum; i++ ) {
            mgr.addTask( new AddSchema( clName + i, schemaName + i ) );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待环境恢复
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );

        // 绑定外部模式成功的集合直接校验数据，绑定外部模式失败的集合再次绑定外部模式然后校验数据
        DBCollection dbcl = null;
        for ( int i = 0; i < schemaNum; i++ ) {
            dbcl = sdb.getCollectionSpace( csName ).getCollection( clName + i );
            BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
            BasicBSONObject selector = new BasicBSONObject();
            selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
            if ( !schemaNames.contains( schemaName + i ) ) {
                dbcl.addSchema( schemaName + i );
            }
            SchemaUtils.checkColumnDef( sdb, schemaName + i, schemaDef );
            SchemaUtils.checkAddSchema( sdb, csName, clName + i,
                    schemaName + i );
            DBCursor cursor = dbcl.query( null, selector, orderBy, null );
            SchemaUtils.checkRecords( cursor, expRecords );
            cursor = dbcl.query( null, null, orderBy, null, 4194304 );
            SchemaUtils.checkRecords( cursor, insertRecords );
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

    private class AddSchema extends OperateTask {
        private String clName;
        private String schemaName;

        private AddSchema( String clName, String schemaName ) {
            this.clName = clName;
            this.schemaName = schemaName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待2秒后再绑定 schema
                int time = new java.util.Random().nextInt( 2000 );
                sleep( time );
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.addSchema( schemaName );
                clNames.add( clName );
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
                        && e.getErrorCode() != SDBError.SDB_CLS_UPDATE_CAT_FAILED
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
