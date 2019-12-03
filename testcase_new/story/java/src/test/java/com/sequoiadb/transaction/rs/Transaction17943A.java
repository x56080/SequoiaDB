package com.sequoiadb.transaction.rs;

import java.util.ArrayList;
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
 * @testcase seqDB-17943:更新事务与读事务并发，更新使用索引扫描，读写操作并发
 * @date 2019-2-28
 * @author yinzhen
 *
 */
@Test(groups = "rs")
public class Transaction17943A extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17943A";
    private DBCollection cl = null;
    private CountDownLatch latch = null;

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

            // 创建索引
            cl.createIndex( "textIndex17943A", indexKey, false, false );

            // 开启3个并发事务
            UpdateThread updateThread1 = new UpdateThread( 50, 0 );
            updateThread1.start();
            UpdateThread updateThread2 = new UpdateThread( 50, 50 );
            updateThread2.start();
            QueryThread queryThread = new QueryThread();
            queryThread.start();

            // 判断事务是返回成功
            Assert.assertTrue( queryThread.isSuccess(),
                    queryThread.getErrorMsg() );
            Assert.assertTrue( updateThread1.isSuccess(),
                    updateThread1.getErrorMsg() );
            Assert.assertTrue( updateThread2.isSuccess(),
                    updateThread2.getErrorMsg() );

            latch.await();
        } catch ( BaseException | InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } finally {

            // 删除索引
            cl.dropIndex( "textIndex17943A" );
        }
    }

    private void insertData() {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 100; i++ ) {
            BSONObject object = ( BSONObject ) JSON
                    .parse( "{a:10000, b:" + i + "}" );
            records.add( object );
        }
        cl.insert( records );
    }

    class UpdateThread extends SdbThreadBase {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private int mutiNum;
        private int addNum;

        public UpdateThread( int mutiNum, int addNum ) {
            super();
            this.addNum = addNum;
            this.mutiNum = mutiNum;
        }

        @Override
        public void exec() throws Exception {
            try {
                int count = 1;
                int endCount = 600;
                while ( true ) {
                    int aid = ( int ) ( Math.random() * mutiNum ) + addNum;
                    int bid = ( int ) ( Math.random() * mutiNum ) + addNum;
                    int value = ( int ) ( Math.random() * 100 ) + 1;

                    // 开启更新事务
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );

                    // 由于更新和读存在死锁，因此需要规避此问题
                    try {
                        cl.update( "{b:" + aid + "}",
                                "{$inc:{a:-" + value + "}}",
                                "{'':'textIndex17943A'}" );
                        cl.update( "{b:" + bid + "}",
                                "{$inc:{a:" + value + "}}",
                                "{'':'textIndex17943A'}" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -13 ) {
                            db.rollback();
                            continue;
                        } else {
                            Assert.fail( e.getMessage() );
                        }
                    }

                    // 提交更新事务
                    db.commit();
                    if ( count == endCount ) {
                        break;
                    } else {
                        count++;
                    }
                    Thread.sleep( 1000 );
                }
            } finally {
                db.commit();
                db.close();
                latch.countDown();
            }
        }
    }

    class QueryThread extends SdbThreadBase {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        @Override
        public void exec() throws Exception {
            try {
                int count = 1;
                int endCount = 3000;
                while ( true ) {

                    // 开启查询事务
                    db.beginTransaction();
                    String sql = "select sum(a) as sum from " + csName + "."
                            + clName + " /*+use_index(textIndex17943A)*/";
                    DBCursor cursor = null;
                    List< BSONObject > actNums = null;
                    try {
                        cursor = db.exec( sql );
                        actNums = TransUtils.getReadActList( cursor );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -13 ) {
                            db.rollback();
                            continue;
                        } else {
                            Assert.fail( e.getMessage() );
                        }
                    }
                    Assert.assertEquals( actNums.size(), 1 );
                    double sumValue = ( double ) actNums.get( 0 ).get( "sum" );
                    int sum = ( int ) sumValue;

                    // 提交查询事务
                    db.commit();
                    if ( sum != 1000000 ) {
                        System.out.println( Thread.currentThread().getName()
                                + " SUM : " + sum + " doTimes : " + count );
                        throw new Exception( "VALUENUM ERROR" );
                    }
                    if ( count == endCount ) {
                        break;
                    } else {
                        count++;
                    }
                    Thread.sleep( 200 );
                }
            } finally {
                db.commit();
                db.closeAllCursors();
                db.close();
                latch.countDown();
            }
        }
    }
}
