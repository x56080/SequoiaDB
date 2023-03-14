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
 * @Description seqDB-30232:并发修改同一个外部模式
 * @Author liuli
 * @Date 2023.03.07
 * @UpdateAuthor liuli
 * @UpdateDate 2023.03.07
 * @version 1.10
 */
public class Schema30232 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs;
    private DBCollection dbcl;
    private String clName = "cl_30232";
    private String schemaName = "schema_30232";

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
        BasicBSONObject schemaDef = ( BasicBSONObject ) JSON.parse(
                "{ a: { Type: 'int32' }, b: { Type: 'int32', WriteDefault: 10 }, c: { Type: 'int32' } }" );
        sdb.createSchema( schemaName, schemaDef );
        dbcl.addSchema( schemaName );
    }

    @Test
    public void test() throws Exception {
        // 插入数据
        ArrayList< BSONObject > insertRecords = new ArrayList<>();

        ArrayList< BSONObject > renameSucRecords = new ArrayList<>();
        ArrayList< BSONObject > dropSucRecords = new ArrayList<>();
        ArrayList< BSONObject > allSucRecords = new ArrayList<>();

        for ( int i = 0; i < 1000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "c", i );
            insertRecords.add( obj );
            // 构造只有 rename 执行成功的预期数据
            BSONObject renameSucObj = new BasicBSONObject();
            renameSucObj.putAll( obj );
            renameSucObj.put( "d", 10 );
            renameSucRecords.add( renameSucObj );
            // 构造只有 drop 执行成功的预期数据
            BSONObject drppSucObj = new BasicBSONObject();
            drppSucObj.put( "c", i );
            drppSucObj.put( "b", 10 );
            dropSucRecords.add( drppSucObj );
            // 构造 rename 和 drop 均执行成功的预期数据
            BSONObject allSucObj = new BasicBSONObject();
            allSucObj.put( "c", i );
            allSucObj.put( "d", 10 );
            allSucRecords.add( allSucObj );

        }
        dbcl.bulkInsert( insertRecords );

        // 外部模式并发对不同字段执行删除字段，添加字段，字段重命名
        ThreadExecutor es = new ThreadExecutor();
        RenameColumn renameColumn = new RenameColumn( schemaName );
        DropColumn dropColumn = new DropColumn( schemaName );
        es.addWorker( renameColumn );
        es.addWorker( dropColumn );
        es.run();

        String schemaDef = "";
        BasicBSONObject selector = new BasicBSONObject();
        selector.put( "_id", new BasicBSONObject( "$include", 0 ) );
        BasicBSONObject orderBy = new BasicBSONObject( "c", 1 );
        if ( renameColumn.getRetCode() != 0 ) {
            // 只有 dropColumn 线程执行成功
            System.out.println( "drop column success" );
            if ( renameColumn.getRetCode() != SDBError.SDB_LOCK_FAILED
                    .getErrorCode()
                    && renameColumn
                            .getRetCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                    .getErrorCode() ) {
                Assert.fail( "renameColumn.getRetCode() : "
                        + renameColumn.getRetCode()
                        + ", the expected result is -147 or -190" );
            }
            Assert.assertEquals( dropColumn.getRetCode(), 0 );
            // 校验外部模式属性
            schemaDef = "{ b: { Type: 'int32', WriteDefault: 10 }, c: { Type: 'int32' } }";
            SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );

            // 校验数据
            SchemaUtils.checkConsistence( sdb, csName, clName, selector,
                    orderBy, dropSucRecords, dropSucRecords );
            SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
        } else if ( dropColumn.getRetCode() != 0 ) {
            // 只有 renameColumn 线程执行成功
            System.out.println( "rename column success" );
            if ( dropColumn.getRetCode() != SDBError.SDB_LOCK_FAILED
                    .getErrorCode()
                    && dropColumn
                            .getRetCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                    .getErrorCode() ) {
                Assert.fail(
                        "dropColumn.getRetCode() : " + dropColumn.getRetCode()
                                + ", the expected result is -147 or -190" );
            }
            Assert.assertEquals( renameColumn.getRetCode(), 0 );
            // 校验外部模式属性
            schemaDef = "{ a: { Type: 'int32' }, d: { Type: 'int32', WriteDefault: 10 }, c: { Type: 'int32' } }";
            SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );

            // 校验数据
            SchemaUtils.checkConsistence( sdb, csName, clName, selector,
                    orderBy, renameSucRecords, renameSucRecords );
            SchemaUtils.checkAddSchema( sdb, csName, clName, schemaName );
        } else {
            // 2个线程都执行成功
            System.out.println( "all success" );
            // 校验外部模式属性
            schemaDef = "{ d: { Type: 'int32', WriteDefault: 10 }, c: { Type: 'int32' } }";
            SchemaUtils.checkColumnDef( sdb, schemaName, schemaDef );

            // 校验数据
            SchemaUtils.checkConsistence( sdb, csName, clName, selector,
                    orderBy, allSucRecords, allSucRecords );
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

    private class RenameColumn extends ResultStore {
        private String schemaName;

        private RenameColumn( String schemaName ) {
            this.schemaName = schemaName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBSchema schema = db.getSchema( schemaName );
                schema.renameColumn( "b", "d" );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class DropColumn extends ResultStore {
        private String schemaName;

        private DropColumn( String schemaName ) {
            this.schemaName = schemaName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBSchema schema = db.getSchema( schemaName );
                schema.dropColumn( "a" );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}