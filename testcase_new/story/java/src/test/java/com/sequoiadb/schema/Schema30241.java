package com.sequoiadb.schema;

import java.util.ArrayList;

import com.sequoiadb.base.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-30241:修改外部模式和插入数据并发
 * @Author liuli
 * @Date 2023.03.08
 * @UpdateAuthor liuli
 * @UpdateDate 2023.03.08
 * @version 1.10
 */
public class Schema30241 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String csName = "cs_30241";
    private String clName = "cl_30241";
    private String schemaName = "schema_30241";

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
        BasicBSONObject schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32' }, b: { Type: 'int32', WriteDefault: 10 } }" );
        sdb.createSchema( schemaName, schemaDef );
        dbcl.addSchema( schemaName );
    }

    @Test
    public void test() throws Exception {
        // 插入数据
        ArrayList< BSONObject > insertRecords = new ArrayList<>();
        ArrayList< BSONObject > renameColumnSucRecords = new ArrayList<>();

        for ( int i = 0; i < 10; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ) );
            // 构造 add schema 执行成功的数据
            renameColumnSucRecords
                    .add( new BasicBSONObject( "a", i ).append( "c", 10 ) );
        }
        System.out.println( "insertRecords -- " + insertRecords );
        dbcl.bulkInsert( insertRecords );

        // 再次构造1000条数据用于插入
        insertRecords.clear();
        for ( int i = 10; i < 20; i++ ) {
            insertRecords.add( new BasicBSONObject( "a", i ) );
            // 构造 add schema 执行成功的数据
            renameColumnSucRecords
                    .add( new BasicBSONObject( "a", i ).append( "c", 10 ) );
        }

        // 并发renameCL和绑定外部模式
        ThreadExecutor es = new ThreadExecutor();
        InsertData insertData = new InsertData( insertRecords );
        RenameColumn renameColumn = new RenameColumn( schemaName );
        es.addWorker( insertData );
        es.addWorker( renameColumn );
        es.run();

        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        BasicBSONObject selector = new BasicBSONObject();
        selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
        String schemaDef = "{ a: { Type: 'int32' }, c: { Type: 'int32', WriteDefault: 10 } }";
        SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );
        SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
        SchemaUtils.checkConsistence( sdb, csName, clName, selector, orderBy,
                renameColumnSucRecords, renameColumnSucRecords );
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
}