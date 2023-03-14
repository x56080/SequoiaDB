package com.sequoiadb.schema;

import java.util.ArrayList;

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
 * @Description seqDB-30233:绑定外部模式和删除CS并发
 * @Author liuli
 * @Date 2023.03.10
 * @UpdateAuthor liuli
 * @UpdateDate 2023.03.10
 * @version 1.10
 */
public class Schema30233 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String csName = "cs_30233";
    private String clName = "cl_30233";
    private String schemaName = "schema_30233";
    private BasicBSONObject schemaDef = new BasicBSONObject();

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
                new BasicBSONObject( "EnableInfoSchema", true ) );
        schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32' }, b: { Type: 'int32', ReadDefault: 10 } }" );
        sdb.createSchema( schemaName, schemaDef );
    }

    @Test
    public void test() throws Exception {
        // 插入数据
        ArrayList< BSONObject > insertRecords = new ArrayList<>();
        ArrayList< BSONObject > expRecords = new ArrayList<>();
        ArrayList< BSONObject > expPrimalRecords = new ArrayList<>();

        for ( int i = 0; i < 1000; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ) );
            expPrimalRecords.add( new BasicBSONObject( "a", i ) );
            expRecords.add( new BasicBSONObject( "a", i ).append( "b", 10 ) );
        }
        dbcl.bulkInsert( insertRecords );

        // 并发renameCL和绑定外部模式
        ThreadExecutor es = new ThreadExecutor();
        DropCS dropCS = new DropCS();
        AddSchema addSchema = new AddSchema( schemaName );
        es.addWorker( dropCS );
        es.addWorker( addSchema );
        es.run();

        if ( dropCS.getRetCode() == 0 ) {
            if ( addSchema.getRetCode() == 0 ) {
                try {
                    sdb.getSchema( schemaName );
                    Assert.fail( "expect schema does not exist" );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_SCHEMA_NOT_EXIST
                            .getErrorCode() ) {
                        throw e;
                    }
                }
            } else if ( addSchema.getRetCode() == SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    || addSchema.getRetCode() == SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode() ) {
                // 外部模式存在，新建一个集合绑定外部模式插入数据并校验
                sdb.getSchema( schemaName );
                dbcs = sdb.createCollectionSpace( csName );
                dbcl = dbcs.createCollection( clName,
                        new BasicBSONObject( "EnableInfoSchema", true ) );
                dbcl.addSchema( schemaName );
                dbcl.bulkInsert( insertRecords );

                // 校验数据
                BasicBSONObject selector = new BasicBSONObject();
                selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
                BasicBSONObject orderBy = new BasicBSONObject( "c", 1 );
                SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );

                SchemaUtils.checkConsistence( sdb, csName, clName, selector,
                        orderBy, expRecords, expPrimalRecords );
                SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
            } else {
                Assert.fail(
                        "addSchema.getRetCode() : " + addSchema.getRetCode()
                                + ", the expected result is -23 or -34" );
            }
        } else if ( dropCS.getRetCode() == SDBError.SDB_LOCK_FAILED
                .getErrorCode()
                || dropCS
                        .getRetCode() == SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode() ) {
            // 校验数据
            BasicBSONObject selector = new BasicBSONObject();
            selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
            BasicBSONObject orderBy = new BasicBSONObject( "c", 1 );
            SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );

            SchemaUtils.checkConsistence( sdb, csName, clName, selector,
                    orderBy, expRecords, expPrimalRecords );
            SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
        } else {
            Assert.fail( "dropCS.getRetCode() : " + dropCS.getRetCode()
                    + ", the expected result is -147 or -190" );
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

    private class AddSchema extends ResultStore {
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
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DropCS extends ResultStore {

        @ExecuteOrder(step = 2)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( csName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}