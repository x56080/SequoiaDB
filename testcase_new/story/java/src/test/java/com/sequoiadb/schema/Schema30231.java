package com.sequoiadb.schema;

import com.sequoiadb.threadexecutor.ResultStore;
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
import com.sequoiadb.base.DBSchema;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

import java.util.ArrayList;

/**
 * @Description seqDB-30231 :: 版本: 1 :: 并发绑定外部模式和修改外部模式
 * @Author huanghaimei
 * @Date 2023.03.08
 * @UpdateAuthor huanghaimei
 * @UpdateDate 2023.03.08
 * @version 1.10
 */
public class Schema30231 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String clName = "cl_30231";
    private String schemaName = "schema_30231";
    // private DBSchema schema;

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
        // 创建外部模式
        BasicBSONObject schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32',ReadDefault:20 }, b: { Type: 'int32' ,ReadDefault:20} }" );
        sdb.createSchema( schemaName, schemaDef );
        // 创建集合
        dbcl = dbcs.createCollection( clName,
                new BasicBSONObject( "ReplSize", 0 ).append( "EnableInfoSchema",
                        true ) );
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
            expRecords.add( new BasicBSONObject( "a", i ).append( "c", 20 ) );
        }
        dbcl.bulkInsert( insertRecords );

        ThreadExecutor es = new ThreadExecutor();
        AddSchema addSchema = new AddSchema( dbcl, schemaName );
        AlterColumn alterColumn = new AlterColumn( schemaName );
        es.addWorker( addSchema );
        es.addWorker( alterColumn );
        es.run();

        String schemaDef = "";
        BasicBSONObject selector = new BasicBSONObject();
        selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );

        schemaDef = "{ a: { Type: 'int32',ReadDefault:20 }, c: { Type: 'int32',ReadDefault:20 } }";
        SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );
        // 校验数据
        SchemaUtils.checkConsistence( sdb, csName, clName, selector, orderBy,
                expRecords, expPrimalRecords );
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
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.addSchema( schemaName );
            }
        }
    }

    private class AlterColumn extends ResultStore {
        private String schemaName;

        private AlterColumn( String schemaName ) {
            this.schemaName = schemaName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBSchema schema = db.getSchema( schemaName );
                schema.renameColumn( "b", "c" );
            }
        }
    }
}