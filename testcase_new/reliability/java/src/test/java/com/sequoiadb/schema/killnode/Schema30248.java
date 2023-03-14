package com.sequoiadb.schema.killnode;

import java.util.ArrayList;

import com.sequoiadb.base.DBSchema;
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
import com.sequoiadb.commlib.*;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.schema.SchemaUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-30248:外部模式删除字段时data备节点异常
 * @Author liuli
 * @Date 2023.03.09
 * @UpdateAuthor liuli
 * @UpdateDate 2023.03.09
 * @version 1.10
 */
public class Schema30248 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private CollectionSpace dbcs = null;
    private String csName = "cs_30248";
    private String clName = "cl_30248";
    private String schemaName = "schema_30248";
    private int schemaNum = 20;
    private String groupName;

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

        groupName = groupMgr.getAllDataGroup().get( 0 ).getGroupName();
        BasicBSONObject schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32' }, b: { Type: 'int32', ReadDefault: 10 } }" );
        dbcs = sdb.createCollectionSpace( csName );
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
        for ( int i = 0; i < 1000; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ) );
        }
        for ( int i = 0; i < schemaNum; i++ ) {
            DBCollection dbcl = dbcs.getCollection( clName + i );
            dbcl.bulkInsert( insertRecords );
        }

        // 外部模式删除字段过程中 data 节点异常
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper slave = dataGroup.getSlave();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( slave.hostName(),
                slave.svcName(), 0 );

        TaskMgr mgr = new TaskMgr( faultTask );
        String columnName = "b";
        for ( int i = 0; i < schemaNum; i++ ) {
            mgr.addTask( new DropColumn( schemaName + i, columnName ) );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待环境恢复
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );

        // 校验数据
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        String schemaDef = "{ a: { Type: 'int32' } }";
        for ( int i = 0; i < schemaNum; i++ ) {
            SchemaUtils.checkColumnDef( sdb, schemaName + i, schemaDef );
            SchemaUtils.checkAddSchema( sdb, csName, clName + i,
                    schemaName + i );
            SchemaUtils.checkConsistence( sdb, csName, clName + i, null,
                    orderBy, insertRecords, insertRecords );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            SchemaUtils.dropSchema( sdb, schemaName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DropColumn extends OperateTask {
        private String schemaName;
        private String columnName;

        private DropColumn( String schemaName, String columnName ) {
            this.schemaName = schemaName;
            this.columnName = columnName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBSchema schema = db.getSchema( schemaName );
                schema.dropColumn( columnName );
            }
        }
    }
}
