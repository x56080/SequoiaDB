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
import com.sequoiadb.base.DBSchema;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-30236:绑定外部模式和truncate并发
 * @Author liuli
 * @Date 2023.03.10
 * @UpdateAuthor liuli
 * @UpdateDate 2023.03.10
 * @version 1.10
 */
public class Schema30236 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String csName = "cs_30236";
    private String clName = "cl_30236";
    private String clNameNew = "cl_new_30236";
    private String schemaName = "schema_30236";

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
        BasicBSONObject schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32' }, b: { Type: 'int32', ReadDefault: 10 } }" );
        sdb.createSchema( schemaName, schemaDef );
    }

    @Test
    public void test() throws Exception {
        // 插入数据
        ArrayList< BSONObject > insertRecords = new ArrayList<>();

        for ( int i = 0; i < 1000; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ) );
        }
        dbcl.bulkInsert( insertRecords );

        // 并发renameCL和绑定外部模式
        ThreadExecutor es = new ThreadExecutor();
        Truncate truncate = new Truncate();
        AddSchema addSchema = new AddSchema( schemaName );
        es.addWorker( truncate );
        es.addWorker( addSchema );
        es.run();

        Assert.assertEquals( dbcl.getCount(), 0 );
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

        private AddSchema( String schemaName ) {
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

    private class Truncate {

        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = db.getCollectionSpace( csName );
                DBCollection dbcl = dbcs.getCollection( clName );
                dbcl.truncate();
            }
        }
    }
}