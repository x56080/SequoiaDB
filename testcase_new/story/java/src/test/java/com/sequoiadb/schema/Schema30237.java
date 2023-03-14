package com.sequoiadb.schema;

import com.sequoiadb.base.DBSchema;
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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;

import java.util.ArrayList;

/**
 * @Description 测试用例 seqDB-30237 :: 版本: 1 :: 绑定外部模式和创建索引并发
 * @Author haunghaimei
 * @Date 2023.03.08
 * @UpdateAuthor huanghaimei
 * @UpdateDate 2023.03.08
 * @version 1.10
 */
public class Schema30237 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String clName = "cl_30237";
    private String schemaName = "schema_30237";
    private DBSchema schema;
    private String indexName = "index_30237";
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
                "{ a: { Type: 'int32' }, b: { Type: 'int32', WriteDefault: 10 } } }" );
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
        CreateIndex createIndex = new CreateIndex();
        es.addWorker( addSchema );
        es.addWorker( createIndex );
        es.run();

        BasicBSONObject selector = new BasicBSONObject();
        selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        // 创建索引成功，绑定外部模式失败
        Assert.assertTrue( dbcl.isIndexExist( indexName ) );
        Assert.assertEquals( createIndex.getRetCode(), 0 );
        if ( addSchema.getRetCode() != 0 ) {
            System.out.println( "绑定外部模式失败！" );
            if ( addSchema.getRetCode() != SDBError.SDB_OPERATION_INCOMPATIBLE
                    .getErrorCode() ) {
                Assert.fail(
                        "addSchema.getRetCode() : " + addSchema.getRetCode()
                                + ", the expected result is -315" );
            }
            dbcl.addSchema( schemaName );
            SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );
            // 校验数据
            SchemaUtils.checkConsistence( sdb, csName, clName, selector,
                    orderBy, expRecords, expPrimalRecords );
            SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
        }
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
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.addSchema( schemaName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class CreateIndex extends ResultStore {
        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                        null );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}