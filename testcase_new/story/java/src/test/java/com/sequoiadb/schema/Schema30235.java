package com.sequoiadb.schema;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-30235:绑定外部模式、修改外部模式和renameCL并发
 * @Author liuli
 * @Date 2023.03.08
 * @UpdateAuthor liuli
 * @UpdateDate 2023.03.08
 * @version 1.10
 */
public class Schema30235 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String csName = "cs_30235";
    private String clName = "cl_30235";
    private String clNameNew = "cl_new_30235";
    private String schemaName = "schema_30235";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        SchemaUtils.dropSchema( sdb, schemaName );

        dbcs = sdb.createCollectionSpace( csName );
        dbcl = dbcs.createCollection( clName,
                new BasicBSONObject( "ReplSize", 0 ).append( "EnableInfoSchema",
                        true ) );
        BasicBSONObject schemaDef = ( BasicBSONObject ) JSON
                .parse( "{ a: { Type: 'int32' }, b: { Type: 'int32' } }" );
        sdb.createSchema( schemaName, schemaDef );
    }

    @Test
    public void test() throws Exception {
        // 插入数据
        ArrayList< BSONObject > insertRecords = new ArrayList<>();
        ArrayList< BSONObject > addSucRecords = new ArrayList<>();
        ArrayList< BSONObject > primalRecords = new ArrayList<>();

        for ( int i = 0; i < 1000; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ).append( "b", i ) );
            // 构造 add schema 执行成功的数据
            primalRecords.add( new BasicBSONObject( "a", i ).append( "b", i ) );
            addSucRecords.add( new BasicBSONObject( "a", i ).append( "b", i )
                    .append( "c", 10 ) );
        }
        dbcl.bulkInsert( insertRecords );

        // 并发renameCL和绑定外部模式
        ThreadExecutor es = new ThreadExecutor();
        RenameCL renameCL = new RenameCL();
        AddSchema addSchema = new AddSchema( schemaName );
        AddColumn addColumn = new AddColumn();
        es.addWorker( renameCL );
        es.addWorker( addSchema );
        es.addWorker( addColumn );
        es.run();

        String schemaDef = "";
        BasicBSONObject selector = new BasicBSONObject();
        selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        if ( addColumn.getRetCode() == 0 ) {
            // 绑定外部模式成功
            System.out.println( "add column success" );
            // 校验外部模式属性
            schemaDef = "{ a: { Type: 'int32' }, b: { Type: 'int32' }, c: { Type: 'int32', ReadDefault: 20 } }";
            SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );

            // 校验数据
            SchemaUtils.checkConsistence( sdb, csName, clNameNew, selector,
                    orderBy, addSucRecords, primalRecords );
            SchemaUtils.checkAddSchema( sdb, csName, clNameNew, schemaName );
        } else if ( addColumn.getRetCode() == SDBError.SDB_LOCK_FAILED
                .getErrorCode()
                || addColumn
                        .getRetCode() == SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode() ) {
            // 绑定外部模式成功
            System.out.println( "add column failed" );
            // 校验外部模式属性
            schemaDef = "{ a: { Type: 'int32' }, b: { Type: 'int32' } }";
            SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );

            // 校验数据
            SchemaUtils.checkConsistence( sdb, csName, clNameNew, selector,
                    orderBy, primalRecords, primalRecords );
            SchemaUtils.checkAddSchema( sdb, csName, clNameNew, schemaName );
        } else {
            Assert.fail( "addColumn.getRetCode() : " + addColumn.getRetCode()
                    + ", expected error code is -147 or -190" );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
                }
            }
        } finally {
            sdb.close();
        }
    }

    private class AddSchema {
        private String schemaName;
        private Sequoiadb db = null;
        private DBCollection dbcl = null;

        private AddSchema( String schemaName ) {
            this.schemaName = schemaName;
        }

        @ExecuteOrder(step = 1)
        private void getCL() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dbcl = db.getCollectionSpace( csName ).getCollection( clName );
        }

        @ExecuteOrder(step = 2)
        private void test() {
            try {
                dbcl.addSchema( schemaName );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class RenameCL {

        @ExecuteOrder(step = 2)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = db.getCollectionSpace( csName );
                dbcs.renameCollection( clName, clNameNew );
            }
        }
    }

    private class AddColumn extends ResultStore {

        @ExecuteOrder(step = 2)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBSchema schema = db.getSchema( schemaName );
                schema.addColumn( "c", new BasicBSONObject( "Type", "int32" )
                        .append( "ReadDefault", 20 ) );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}