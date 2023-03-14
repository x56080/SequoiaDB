package com.sequoiadb.schema;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;

import java.util.ArrayList;
import java.util.Collection;

/**
 * @Description seqDB-30234 :: 版本: 1 :: 绑定外部模式和删除CL并发
 * @Author haunghaimei
 * @Date 2023.03.08
 * @UpdateAuthor huanghaimei
 * @UpdateDate 2023.03.08
 * @version 1.10
 */
public class Schema30234 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String csName = "cs_30234";
    private String clName = "cl_30234";
    private String schemaName = "schema_30234";
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

        // 并发dropCL和绑定外部模式
        ThreadExecutor es = new ThreadExecutor();
        DropCL dropCL = new DropCL();
        AddSchema addSchema = new AddSchema( schemaName );
        es.addWorker( dropCL );
        es.addWorker( addSchema );
        es.run();

        // 删除cl和添加schame都成功
        if ( dropCL.getRetCode() == 0 ) {
            if ( addSchema.getRetCode() == 0 ) {
                try {
                    // 外部模式被删除，获取不到外部模式
                    sdb.getSchema( schemaName );
                    Assert.fail( "expect schema does not exist" );
                } catch ( BaseException e ) {
                    // 获取外部模式错误码如果不是报错-397,则抛错
                    if ( e.getErrorCode() != SDBError.SDB_SCHEMA_NOT_EXIST
                            .getErrorCode() ) {
                        throw e;
                    }
                }
                // 删除cl成功，绑定schema失败
            } else if ( addSchema.getRetCode() == SDBError.SDB_DMS_NOTEXIST
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
                BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
                SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );

                SchemaUtils.checkConsistence( sdb, csName, clName, selector,
                        orderBy, expRecords, expPrimalRecords );
                SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
            } else {
                Assert.fail(
                        "addSchema.getRetCode() : " + addSchema.getRetCode()
                                + ", the expected result is -23" );
            }
        } else if ( dropCL.getRetCode() == SDBError.SDB_LOCK_FAILED
                .getErrorCode()
                || dropCL
                        .getRetCode() == SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode() ) {
            // 校验数据
            BasicBSONObject selector = new BasicBSONObject();
            selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
            BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
            SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );

            SchemaUtils.checkConsistence( sdb, csName, clName, selector,
                    orderBy, expRecords, expPrimalRecords );
            SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
        } else {
            Assert.fail( "dropCL.getRetCode() : " + dropCL.getRetCode()
                    + ", the expected result is -147 or -190" );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
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

    private class DropCL extends ResultStore {

        @ExecuteOrder(step = 2)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = db.getCollectionSpace( csName );
                dbcs.dropCollection( clName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}