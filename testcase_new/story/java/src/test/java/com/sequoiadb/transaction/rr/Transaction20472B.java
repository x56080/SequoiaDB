package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20472：转账的同时，创建删除索引
 * @date 2020-1-16
 * @author zhaoyu
 *
 */

@Test(groups = "rr")
public class Transaction20472B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl20472B";
    private String idxName = "idx20472B";
    private DBCollection cl = null;
    private String indexKey = null;
    private int insertNum = 100;
    private int loopNum = 1000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb.updateConfig( ( BSONObject ) JSON.parse( "{diaglevel:5}" ) );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertData();
    }

    @AfterClass
    public void tearDown() {
        sdb.updateConfig( ( BSONObject ) JSON.parse( "{diaglevel:3}" ) );
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'b':-1}" }, { "{'b':1}" } };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey ) throws Exception {
        this.indexKey = indexKey;

        // 开启 3 个并发事务
        ThreadExecutor threadExecutor = new ThreadExecutor( 3600000 );
        threadExecutor.addWorker( new InsertDeleteThread() );
        threadExecutor.addWorker( new QueryThread() );
        threadExecutor.addWorker( new DropIndexThread() );
        threadExecutor.run();
    }

    private void insertData() {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < insertNum; i++ ) {
            BSONObject object = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:10000, b:" + i + "}" );
            records.add( object );
        }
        cl.insert( records );
    }

    private class InsertDeleteThread {
        private Sequoiadb db = CommLib.getRandomSequoiadb();

        @ExecuteOrder(step = 1, desc = "插入删除同一条记录")
        private void insertDelete() {
            try {
                for ( int i = 0; i < loopNum * 2; i++ ) {
                    System.out.println( "insert delete times:" + i );
                    int aId = ( int ) ( Math.random() * insertNum ) + insertNum;
                    int bId = ( int ) ( Math.random() * insertNum );
                    int cId = ( int ) ( Math.random() * insertNum ) - insertNum;

                    int aBalance = aId + 10000;
                    int bBalance = bId + 10000;
                    int cBalance = cId + 10000;

                    // 开启更新事务
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    try {
                        BSONObject object = ( BSONObject ) JSON
                                .parse( "{_id:" + aId + ", a:" + aBalance
                                        + ", b:" + aId + "}" );
                        cl.insert( object );
                        cl.delete( "{b:" + aId + "}",
                                "{'':'" + idxName + "'}" );

                        object = ( BSONObject ) JSON.parse(
                                "{_id:" + ( bId + insertNum * 2 ) + ", a:"
                                        + bBalance + ", b:" + bId + "}" );
                        cl.insert( object );
                        cl.delete( "{_id:" + ( bId + insertNum * 2 ) + "}",
                                "{'':'$id'}" );

                        object = ( BSONObject ) JSON.parse( "{_id:" + cId
                                + ", a:" + cBalance + ", b:" + cId + "}" );
                        cl.insert( object );
                        cl.delete( "{b:" + cId + "}",
                                "{'':'" + idxName + "'}" );

                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -47 || e.getErrorCode() == -48
                                || e.getErrorCode() == -52
                                || e.getErrorCode() == -10
                                || e.getErrorCode() == -199 ) {
                            db.rollback();
                            continue;
                        } else {
                            Assert.fail( e.getMessage() );
                        }
                    }

                    // 提交更新事务
                    db.commit();

                }
            } finally {
                db.commit();
                db.close();
                System.out.println( "insert delete thread end" + new Date() );
            }
        }
    }

    private class QueryThread {
        private Sequoiadb db = CommLib.getRandomSequoiadb();

        @ExecuteOrder(step = 1, desc = "查询记录总账")
        private void query() throws Exception {
            try {
                for ( int i = 0; i < loopNum * 3; i++ ) {
                    System.out.println( "query times:" + i );
                    // 开启查询事务，索引扫描
                    db.beginTransaction();
                    String sqlIdxScan = "select sum(a) as sum from " + csName
                            + "." + clName + " /*+use_index(NULL)*/";
                    DBCursor cursor = null;
                    List< BSONObject > actNums = null;
                    cursor = db.exec( sqlIdxScan );
                    actNums = TransUtils.getReadActList( cursor );
                    Assert.assertEquals( actNums.size(), 1 );
                    double sumValue = ( double ) actNums.get( 0 ).get( "sum" );
                    int sum = ( int ) sumValue;
                    db.commit();
                    if ( sum != 1000000 ) {
                        System.out.println( "TblScan Sum Value: " + sum );
                        throw new Exception(
                                "TblScan check sum error, expect sum is 1000000, but actual sum:"
                                        + sum );
                    }

                    // 开启查询事务，表扫描
                    db.beginTransaction();
                    String sqlTblScan = "select sum(a) as sum from " + csName
                            + "." + clName + " /*+use_index(" + idxName + ")*/";
                    try {
                        cursor = db.exec( sqlTblScan );
                        actNums = TransUtils.getReadActList( cursor );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -48 || e.getErrorCode() == -47
                                || e.getErrorCode() == -52
                                || e.getErrorCode() == -10
                                || e.getErrorCode() == -199 ) {
                            db.rollback();
                            continue;
                        } else {
                            Assert.fail( e.getMessage() );
                        }
                    }

                    Assert.assertEquals( actNums.size(), 1 );
                    sumValue = ( double ) actNums.get( 0 ).get( "sum" );
                    sum = ( int ) sumValue;
                    db.commit();
                    if ( sum != 1000000 ) {
                        System.out.println( "IdxScan Sum Value: " + sum );
                        throw new Exception(
                                "IdxScan check sum error, expect sum is 1000000, but actual sum:"
                                        + +sum );
                    }
                }
            } finally {
                db.commit();
                db.closeAllCursors();
                db.close();
                System.out.println( "query thread end" + new Date() );
            }
        }
    }

    private class DropIndexThread {
        private Sequoiadb db = CommLib.getRandomSequoiadb();

        @ExecuteOrder(step = 1, desc = "删除索引")
        private void dropIndex() {
            try {
                for ( int i = 0; i < loopNum * 3; i++ ) {
                    System.out.println( "drop and create index:" + i );
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    cl.createIndex( idxName, indexKey, false, false );
                    Assert.assertTrue( cl.isIndexExist( idxName ) );
                    cl.dropIndex( idxName );
                    Assert.assertFalse( cl.isIndexExist( idxName ) );

                }
            } finally {
                db.commit();
                db.close();
                System.out
                        .println( "create drop index thread end" + new Date() );
            }
        }
    }
}
