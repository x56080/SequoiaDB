package com.sequoiadb.schema;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBSchema;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;

/**
 * @Descreption seqDB-30240:修改外部模式和truncate并发
 * @Author huanghaimei
 * @CreateDate 2023/3/8
 * @UpdateUser huanghaimei
 * @UpdateDate 2023/3/8
 * @Version 1.10
 */
public class Schema30240 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String clName = "cl_30240";
    private String schemaName = "schema_30240";
    private DBSchema schema;

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
        BasicBSONObject schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32' }, b: { Type: 'int32', WriteDefault: 10 } }" );
        schema = sdb.createSchema( schemaName, schemaDef );
    }

    @Test
    public void test() throws Exception {
        dbcl.addSchema( schemaName );
        // 插入数据
        ArrayList< BSONObject > insertRecords = new ArrayList<>();
        ArrayList< BSONObject > expRecords = new ArrayList<>();
        ArrayList< BSONObject > expPrimalRecords = new ArrayList<>();
        for ( int i = 0; i < 10; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ) );
        }
        dbcl.bulkInsert( insertRecords );

        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new RenameColumn( schemaName ) );
        es.addWorker( new Truncate() );
        es.run();

        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        BasicBSONObject selector = new BasicBSONObject();
        selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
        String schemaDef = "{ a: { Type: 'int32' }, c: { Type: 'int32', WriteDefault: 10 } }";
        SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );
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

    private class RenameColumn {
        private String schemaName;

        private RenameColumn( String schemaName ) {
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

    private class Truncate {
        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.truncate();
            }
        }
    }

}
