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
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-30229 :: 版本: 1 :: 并发创建相同的外部模式
 * @Author liuli
 * @Date 2023.03.07
 * @UpdateAuthor liuli
 * @UpdateDate 2023.03.07
 * @version 1.10
 */
public class Schema30229 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String clName = "cl_30229";
    private String schemaName = "schema_30229";
    private int isSuccessCount = 0;

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
    }

    @Test
    public void test() throws Exception {
        BasicBSONObject schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32' }, b: { Type: 'int32', ReadDefault: 10 } }" );
        ThreadExecutor es = new ThreadExecutor();
        int threadNum = 20;
        for ( int i = 0; i < threadNum; i++ ) {
            es.addWorker( new CreateSchema( schemaName, schemaDef ) );
        }
        es.run();

        SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );

        ArrayList< BSONObject > insertRecords = new ArrayList<>();
        ArrayList< BSONObject > expPrimalRecords = new ArrayList<>();
        for ( int i = 0; i < 1000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            insertRecords.add( obj );
            obj.put( "b", 10 );
            expPrimalRecords.add( obj );
        }
        dbcl.bulkInsert( insertRecords );
        dbcl.addSchema( schemaName );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        SchemaUtils.checkConsistence( sdb, csName, clName, null, orderBy,
                insertRecords, expPrimalRecords );
        SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
        Assert.assertEquals( isSuccessCount, 1 );
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

    private class CreateSchema {
        private String schemaName;
        private BasicBSONObject schemaDef;

        private CreateSchema( String schemaName, BasicBSONObject schemaDef ) {
            this.schemaName = schemaName;
            this.schemaDef = schemaDef;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.createSchema( schemaName, schemaDef );
                isSuccessCount++;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_SCHEMA_EXIST
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}