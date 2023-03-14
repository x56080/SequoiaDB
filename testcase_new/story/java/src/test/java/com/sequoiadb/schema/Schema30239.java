package com.sequoiadb.schema;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-30239:绑定外部模式和插入数据并发
 * @Author liuli
 * @Date 2023.03.10
 * @UpdateAuthor liuli
 * @UpdateDate 2023.03.10
 * @version 1.10
 */
public class Schema30239 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private String csName = "cs_30239";
    private String clName = "cl_30239";
    private String schemaName = "schema_30239";
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
        dbcs.createCollection( clName, new BasicBSONObject( "ReplSize", 0 )
                .append( "EnableInfoSchema", true ) );
        schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32' }, c: { Type: 'int32', ReadDefault: 10 } }" );
        sdb.createSchema( schemaName, schemaDef );
    }

    @Test(enabled = false)
    public void test() throws Exception {
        // 插入数据
        ArrayList< BSONObject > insertRecords = new ArrayList<>();
        ArrayList< BSONObject > addSchemaSucRecords = new ArrayList<>();
        ArrayList< BSONObject > addSchemaSucPrimalRecords = new ArrayList<>();

        // 再次构造1000条数据用于插入
        for ( int i = 0; i < 1000; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ) );
            // 构造 add schema 执行成功的数据
            addSchemaSucRecords
                    .add( new BasicBSONObject( "a", i ).append( "c", 10 ) );
            addSchemaSucPrimalRecords.add( new BasicBSONObject( "a", i ) );
        }

        // 并发renameCL和绑定外部模式
        ThreadExecutor es = new ThreadExecutor();
        InsertData insertData = new InsertData( insertRecords );
        AddSchema addSchema = new AddSchema();
        es.addWorker( insertData );
        es.addWorker( addSchema );
        es.run();

        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        BasicBSONObject selector = new BasicBSONObject();
        selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
        SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );
        SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
        SchemaUtils.checkConsistence( sdb, csName, clName, selector, orderBy,
                addSchemaSucRecords, addSchemaSucPrimalRecords );
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

    private class InsertData {
        private ArrayList< BSONObject > insertRecords;

        private InsertData( ArrayList< BSONObject > insertRecords ) {
            this.insertRecords = insertRecords;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.bulkInsert( insertRecords );
            }
        }
    }

    private class AddSchema {

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
}