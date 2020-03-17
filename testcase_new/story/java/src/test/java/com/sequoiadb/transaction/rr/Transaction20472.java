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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20472：转账的同时，创建删除索引
 * @date 2020-1-16
 * @author zhaoyu
 *
 */

@Test(groups = "rr")
public class Transaction20472 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl20472";
    private String idxName = "idx20472";
    private DBCollection cl = null;
    private CountDownLatch latch = null;
    private String indexKey = null;
    private int insertNum = 100;
    private int loopNum = 1000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertData();
    }

    @AfterClass
    public void tearDown() {
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
            this.indexKey = indexKey;

            // 创建索引
            cl.createIndex( idxName, indexKey, false, false );

            // 开启 4 个并发事务
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();

            InsertDeleteThread insertDeleteTh = new InsertDeleteThread();
            insertDeleteTh.start();

            QueryThread queryThread = new QueryThread();
            queryThread.start();

            DropIndexThread dropIndexThread = new DropIndexThread();
            dropIndexThread.start();

            // 判断事务是否正确返回
            Assert.assertTrue( queryThread.isSuccess(),
                    queryThread.getErrorMsg() );
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );
            Assert.assertTrue( insertDeleteTh.isSuccess(),
                    insertDeleteTh.getErrorMsg() );
            Assert.assertTrue( dropIndexThread.isSuccess(),
                    dropIndexThread.getErrorMsg() );

            latch.await();
        } catch ( InterruptedException e ) {
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
                for ( int i = 0; i < loopNum; i++ ) {
                    System.out.println( "update times:" + i );
                    int aid = ( int ) ( Math.random() * insertNum );
                    int bid = ( int ) ( Math.random() * insertNum );
                    int value = ( int ) ( Math.random() * 100 ) + 1;

                    // 开启更新事务
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );

                    // 由于更新和读存在死锁，因此需要规避此问题
                    try {
                        cl.update( "{b:" + aid + "}",
                                "{$inc:{a:-" + value + "}}",
                                "{'':'" + idxName + "'}" );
                        cl.update( "{b:" + bid + "}",
                                "{$inc:{a:" + value + "}}",
                                "{'':'" + idxName + "'}" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -13 || e.getErrorCode() == -48
                                || e.getErrorCode() == -52
                                || e.getErrorCode() == -10
                                || e.getErrorCode() == -199 ) {
                            db.rollback();
                            continue;
                        } else {
                            e.printStackTrace();
                            throw e;
                        }
                    }
                    // 提交更新事务
                    db.commit();
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
                for ( int i = 0; i < loopNum; i++ ) {
                    System.out.println( "insert delete times:" + i );
                    int aId = ( int ) ( Math.random() * insertNum ) + insertNum;
                    int bId = ( int ) ( Math.random() * insertNum );
                    int cId = ( int ) ( Math.random() * insertNum ) - insertNum;

                    // 开启更新事务
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    try {
                        BSONObject object = ( BSONObject ) JSON.parse(
                                "{_id:" + aId + ", a:10000, b:" + aId + "}" );
                        cl.insert( object );
                        cl.delete( "{b:" + aId + "}",
                                "{'':'" + idxName + "'}" );

                        object = ( BSONObject ) JSON
                                .parse( "{_id:" + ( bId + insertNum * 2 )
                                        + ", a:10000, b:" + bId + "}" );
                        cl.insert( object );
                        cl.delete( "{_id:" + ( bId + insertNum * 2 ) + "}",
                                "{'':'$id'}" );

                        object = ( BSONObject ) JSON.parse(
                                "{_id:" + cId + ", a:10000, b:" + cId + "}" );
                        cl.insert( object );
                        cl.delete( "{b:" + cId + "}",
                                "{'':'" + idxName + "'}" );

                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -13 || e.getErrorCode() == -48
                                || e.getErrorCode() == -52
                                || e.getErrorCode() == -10
                                || e.getErrorCode() == -199 ) {
                            e.printStackTrace();
                            throw e;
                        }

                    }

                    // 提交更新事务
                    db.commit();

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
                for ( int i = 0; i < loopNum * 5; i++ ) {
                    System.out.println( "query times:" + i );
                    // 开启查询事务
                    db.beginTransaction();
                    String sql = "select sum(a) as sum from " + csName + "."
                            + clName;
                    DBCursor cursor = null;
                    List< BSONObject > actNums = null;
                    try {
                        cursor = db.exec( sql );
                        actNums = TransUtils.getReadActList( cursor );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -48 || e.getErrorCode() == -52
                                || e.getErrorCode() == -10
                                || e.getErrorCode() == -199 ) {
                            e.printStackTrace();
                            Assert.fail( e.getMessage() );

                        }
                    }
                    Assert.assertEquals( actNums.size(), 1 );
                    double sumValue = ( double ) actNums.get( 0 ).get( "sum" );
                    int sum = ( int ) sumValue;

                    // 提交查询事务
                    db.commit();
                    if ( sum != 1000000 ) {
                        System.out.println( "SUM Value: " + sum );
                        throw new Exception( "VALUENUM ERROR" );
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

    class DropIndexThread extends SdbThreadBase {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        @Override
        public void exec() throws Exception {
            try {
                for ( int i = 0; i < loopNum * 3; i++ ) {
                    System.out.println( "drop and create index:" + i );
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    Assert.assertTrue( cl.isIndexExist( idxName ) );
                    cl.dropIndex( idxName );
                    Assert.assertFalse( cl.isIndexExist( idxName ) );
                    cl.createIndex( idxName, indexKey, false, false );

                }
            } finally {
                db.commit();
                db.close();
                latch.countDown();
                System.out
                        .println( "create drop index thread end" + new Date() );
            }
        }
    }
}
