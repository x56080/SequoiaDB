package com.sequoiadb.sessionaccess;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @TestLink: seqDB-14147
 * @describe: 设置timeout值，执行操作超时
 * @author wangkexin
 * @Date 2019.02.16
 * @version 1.00
 */
public class SessionAccess14147 extends SdbTestBase {
    private String clname = "cl14147";
    private Sequoiadb db;
    private DBCollection dbcl;
    private String mainCSName = "mainCS14147";
    private String subCSName = "subCS14147";
    private String rgName = "sessionAccessRG14147";

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }
        SessionAccessUtil.createRG( db, rgName );
        BSONObject options = new BasicBSONObject( "Group", rgName );
        options.put( "ReplSize", 0 );
        dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clname, options );
        SessionAccessUtil.insertRecords( dbcl );
    }

    @Test
    public void test14147() {
        // 创建、删除cs,cl
        String csName = "14147CS", clName = "14147cl";
        db.setSessionAttr( new BasicBSONObject( "Timeout", 1000L ) );
        try {
            db.createCollectionSpace( csName ).createCollection( clName );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -13 )
                throw e;
        } finally {
            db.setSessionAttr( new BasicBSONObject( "Timeout", -1L ) );
            db.dropCollectionSpace( csName );
        }

        db.getCollectionSpace( SdbTestBase.csName ).createCollection( clName );
        db.setSessionAttr( new BasicBSONObject( "Timeout", 1000L ) );
        try {
            db.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -13 )
                throw e;
        } finally {
            db.setSessionAttr( new BasicBSONObject( "Timeout", -1L ) );
            if ( db.getCollectionSpace( SdbTestBase.csName )
                    .isCollectionExist( clName ) ) {
                db.getCollectionSpace( SdbTestBase.csName )
                        .dropCollection( clName );
            }
        }

        // 挂载、修改cl
        // create mainCL and subCL
        String mainCLName = "mainCL14147";
        BSONObject mainCLOption = new BasicBSONObject();
        BSONObject mainShardingKey = new BasicBSONObject();
        mainShardingKey.put( "a", 1 );
        mainCLOption.put( "ShardingKey", mainShardingKey );
        mainCLOption.put( "ShardingType", "range" );
        mainCLOption.put( "ReplSize", 0 );
        mainCLOption.put( "IsMainCL", true );
        mainCLOption.put( "Compressed", true );

        String subCLName = "subCL14147";
        BSONObject subCLOption = new BasicBSONObject();
        BSONObject subShardingKey = new BasicBSONObject();
        subShardingKey.put( "b", 1 );
        subCLOption.put( "ShardingKey", subShardingKey );

        CollectionSpace mainCS = db.createCollectionSpace( mainCSName );
        CollectionSpace subCS = db.createCollectionSpace( subCSName );
        DBCollection mainCL = mainCS.createCollection( mainCLName,
                mainCLOption );
        subCS.createCollection( subCLName, subCLOption );

        // attach
        BSONObject attachOption = new BasicBSONObject();
        BSONObject attachLowObj = new BasicBSONObject();
        BSONObject attachUpObj = new BasicBSONObject();
        attachOption.put( "a", 1 );
        attachOption.put( "LowBound", attachLowObj );
        attachUpObj.put( "a", 100 );
        attachOption.put( "UpBound", attachUpObj );

        db.setSessionAttr( new BasicBSONObject( "Timeout", 1000L ) );
        try {
            mainCL.attachCollection( subCSName + "." + subCLName,
                    attachOption );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -13 )
                throw e;
        } finally {
            db.setSessionAttr( new BasicBSONObject( "Timeout", -1L ) );
            db.dropCollectionSpace( mainCSName );
            db.dropCollectionSpace( subCSName );
        }

        // 修改 cl
        db.getCollectionSpace( SdbTestBase.csName ).createCollection( clName );
        db.setSessionAttr( new BasicBSONObject( "Timeout", 1000L ) );
        try {
            BSONObject opt = new BasicBSONObject();
            opt.put( "ReplSize", -1 );
            db.getCollectionSpace( SdbTestBase.csName ).getCollection( clName )
                    .alterCollection( opt );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -13 )
                throw e;
        } finally {
            db.setSessionAttr( new BasicBSONObject( "Timeout", -1L ) );
            db.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
        }

        // 执行插入、更新、删除操作
        db.setSessionAttr( new BasicBSONObject( "Timeout", 1000L ) );
        try {
            for ( int i = 0; i < 100; i++ ) {
                BSONObject record = new BasicBSONObject();
                record.put( "a", "test14147_" + i );
                dbcl.insert( record );
            }
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -13 )
                throw e;
        } finally {
            db.setSessionAttr( new BasicBSONObject( "Timeout", -1L ) );
        }

        BSONObject modifier = new BasicBSONObject();
        BSONObject modifyObj = new BasicBSONObject();
        BSONObject match = new BasicBSONObject();
        modifyObj.put( "a", "14147" );
        modifier.put( "$set", modifyObj );
        db.setSessionAttr( new BasicBSONObject( "Timeout", 1000L ) );
        try {
            dbcl.update( match, modifier, null );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -13 )
                throw e;
        } finally {
            db.setSessionAttr( new BasicBSONObject( "Timeout", -1L ) );
        }

        db.setSessionAttr( new BasicBSONObject( "Timeout", 1000L ) );
        try {
            BSONObject matcher = new BasicBSONObject();
            matcher.put( "a", "test14147_10" );
            dbcl.delete( matcher );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -13 )
                throw e;
        } finally {
            db.setSessionAttr( new BasicBSONObject( "Timeout", -1L ) );
        }
    }

    @AfterClass
    public void teardown() throws InterruptedException {
        db.getCollectionSpace( SdbTestBase.csName ).dropCollection( clname );
        db.removeReplicaGroup( rgName );
        db.close();
    }
}
