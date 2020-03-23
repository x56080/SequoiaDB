package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.concurrent.CountDownLatch;

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
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20471：转账程序支持全局RR隔离级别
 * @date 2020-1-16
 * @author zhaoyu
 *
 */

@Test(groups = "rr")
public class Transaction20471 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl20471";
    private String idxName = "idx20471";
    private DBCollection cl = null;
    private CountDownLatch latch = null;
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
    public void test( String indexKey ) {
        try {
            latch = new CountDownLatch( 3 );

            // 创建索引
            cl.createIndex( idxName, indexKey, false, false );

            // 开启 3 个并发事务
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();

            InsertDeleteThread insertDeleteTh = new InsertDeleteThread();
            insertDeleteTh.start();

            QueryThread queryThread = new QueryThread();
            queryThread.start();

            // 判断事务是否正确返回
            Assert.assertTrue( queryThread.isSuccess(),
                    queryThread.getErrorMsg() );
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );
            Assert.assertTrue( insertDeleteTh.isSuccess(),
                    insertDeleteTh.getErrorMsg() );
            latch.await();
        } catch ( InterruptedException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            // 删除索引
            cl.dropIndex( idxName );
        }
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

    class UpdateThread extends SdbThreadBase {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        @Override
        public void exec() throws Exception {
            try {
                for ( int i = 0; i < loopNum * 3; i++ ) {
                    System.out.println( "update times:" + i );
                    int aid = ( int ) ( Math.random() * insertNum );
                    int bid = ( int ) ( Math.random() * insertNum );
                    int value = ( int ) ( Math.random() * 100 ) + 1;

                    // 开启更新事务
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    cl.update( "{b:" + aid + "}", "{$inc:{a:-" + value + "}}",
                            "{'':'" + idxName + "'}" );
                    cl.update( "{b:" + bid + "}", "{$inc:{a:" + value + "}}",
                            "{'':'" + idxName + "'}" );
                    // 提交、回滚更新事务
                    if ( aid % 2 == 0 ) {
                        db.commit();
                    } else {
                        db.rollback();
                    }

                }
            } finally {
                db.commit();
                db.close();
                latch.countDown();
                System.out.println( "udpate thread end" + new Date() );
            }
        }
    }

    class InsertDeleteThread extends SdbThreadBase {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        @Override
        public void exec() throws Exception {
            try {
                for ( int i = 0; i < loopNum * 2; i++ ) {
                    System.out.println( "insert delete times:" + i );
                    int aId = ( int ) ( Math.random() * insertNum ) + insertNum;
                    int bId = ( int ) ( Math.random() * insertNum );
                    int cId = ( int ) ( Math.random() * insertNum ) - insertNum;

                    int aBalance = aId + 10000;
                    int bBalance = bId + 10000;
                    int cBalance = cId + 10000;

                    // 开启写事务
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    BSONObject object = ( BSONObject ) JSON.parse( "{_id:" + aId
                            + ", a:" + aBalance + ", b:" + aId + "}" );
                    cl.insert( object );
                    cl.delete( "{b:" + aId + "}", "{'':'" + idxName + "'}" );

                    object = ( BSONObject ) JSON
                            .parse( "{_id:" + ( bId + insertNum * 2 ) + ", a:"
                                    + bBalance + ", b:" + bId + "}" );
                    cl.insert( object );
                    cl.delete( "{_id:" + ( bId + insertNum * 2 ) + "}",
                            "{'':'$id'}" );

                    object = ( BSONObject ) JSON.parse( "{_id:" + cId + ", a:"
                            + cBalance + ", b:" + cId + "}" );
                    cl.insert( object );
                    cl.delete( "{b:" + cId + "}", "{'':'" + idxName + "'}" );

                    // 提交、回滚更新事务
                    if ( aId % 2 == 0 ) {
                        db.commit();
                    } else {
                        db.rollback();
                    }
                }
            } finally {
                db.commit();
                db.close();
                latch.countDown();
                System.out.println( "insert delete thread end" + new Date() );
            }
        }
    }

    class QueryThread extends SdbThreadBase {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        @Override
        public void exec() throws Exception {
            try {
                for ( int i = 0; i < loopNum; i++ ) {
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
                    cursor = db.exec( sqlTblScan );
                    actNums = TransUtils.getReadActList( cursor );
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
                latch.countDown();
                System.out.println( "query thread end" + new Date() );
            }
        }
    }
}
