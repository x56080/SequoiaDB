package com.sequoiadb.schema;

import java.util.ArrayList;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
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
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-30230:并发绑定外部模式和删除外部模式
 * @Author huanghaimei
 * @Date 2023.03.08
 * @UpdateAuthor huanghaimei
 * @UpdateDate 2023.03.08
 * @version 1.10
 */
public class Schema30230 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String clName = "cl_30230";
    private String schemaName = "schema_30230";
    private BasicBSONObject schemaDef;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        dbcs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( dbcs.isCollectionExist( clName ) ) {
            dbcs.dropCollection( clName );
        }
        SchemaUtils.dropSchema( sdb, schemaName );

        dbcl = dbcs.createCollection( clName,
                new BasicBSONObject( "ReplSize", 0 ).append( "EnableInfoSchema",
                        true ) );
        // 创建外部模式
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
        for ( int i = 0; i < 10; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ) );
            expPrimalRecords.add( new BasicBSONObject( "a", i ) );
            expRecords.add( new BasicBSONObject( "a", i ).append( "b", 10 ) );
        }
        dbcl.bulkInsert( insertRecords );

        ThreadExecutor es = new ThreadExecutor();
        AddSchema addSchema = new AddSchema( dbcl, schemaName );
        DropSchema dropSchema = new DropSchema( schemaName );
        es.addWorker( addSchema );
        es.addWorker( dropSchema );
        es.run();

        BasicBSONObject selector = new BasicBSONObject();
        selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );

        if ( addSchema.getRetCode() != 0 ) {
            // 只有dropSchema执行成功了
            System.out.println( "drop schema success" );
            if ( addSchema.getRetCode() != SDBError.SDB_SCHEMA_NOT_EXIST
                    .getErrorCode() ) {
                Assert.fail(
                        "addSchema.getRetCode() : " + addSchema.getRetCode()
                                + ", the expected result is  -397" );
            }
            sdb.createSchema( schemaName, schemaDef );
            dbcl.addSchema( schemaName );

            SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );
            // 校验数据
            SchemaUtils.checkConsistence( sdb, csName, clName, selector,
                    orderBy, expRecords, expPrimalRecords );
            SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
        } else if ( dropSchema.getRetCode() != 0 ) {
            // 只有 addSchema 线程执行成功
            System.out.println( "add schema success" );
            if ( dropSchema.getRetCode() != SDBError.SDB_SCHEMA_NOT_EXIST
                    .getErrorCode()
                    && dropSchema
                            .getRetCode() != SDBError.SDB_OPERATION_INCOMPATIBLE
                                    .getErrorCode() ) {
                Assert.fail(
                        "dropSchema.getRetCode() : " + dropSchema.getRetCode()
                                + ", the expected result is -397 and -315" );
            }
            SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );
            // 校验数据
            SchemaUtils.checkConsistence( sdb, csName, clName, selector,
                    orderBy, expRecords, expPrimalRecords );
            SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                dbcs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }

    private class AddSchema extends ResultStore {
        private DBCollection dbcl;
        private String schemaName;

        private AddSchema( DBCollection collection, String schemaName ) {
            this.dbcl = collection;
            this.schemaName = schemaName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                dbcl.addSchema( schemaName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class DropSchema extends ResultStore {
        private String schemaName;

        private DropSchema( String schemaName ) {
            this.schemaName = schemaName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropSchema( schemaName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}