package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.Collections;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17945:插入删除事务与读事务并发，删除使用索引扫描，读写操作并发
 * @date 2019-2-28
 * @author yinzhen
 *
 */
@Test(groups = "rc")
public class Transaction17945 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17945";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > expList2 = null;
    private CountDownLatch latch = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        expList = insertData();
        expList2 = new ArrayList<>( expList );
        Collections.reverse( expList2 );
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
        return new Object[][] { { "{'b':1}" }/* , { "{'b':-1}" } */ };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey ) {
        try {
            latch = new CountDownLatch( 3 );

            // 创建索引
            cl.createIndex( "textIndex17945", indexKey, false, false );

            // 开启3个并发事务
            UpdateThread updateThread1 = new UpdateThread( 100 );
            updateThread1.start();
            UpdateThread updateThread2 = new UpdateThread( 200 );
            updateThread2.start();
            QueryThread queryThread = new QueryThread();
            queryThread.start();

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
            cl.dropIndex( "textIndex17945" );
        }
    }

    private List< BSONObject > insertData() {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 100; i++ ) {
            BSONObject object = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:10000, b:" + i + "}" );
            records.add( object );
        }
        cl.insert( records );
        return records;
    }

    class UpdateThread extends SdbThreadBase {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private int addNum;

        public UpdateThread( int addNum ) {
            super();
            this.addNum = addNum;
        }

        @Override
        public void exec() throws Exception {
            try {
                int count = 1;
                int endCount = 6;
                while ( true ) {
                    int value = ( int ) ( Math.random() * 100 ) + addNum;

                    // 开启更新事务
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    BSONObject object = ( BSONObject ) JSON.parse(
                            "{_id:" + value + ", a:10000, b:" + value + "}" );
                    cl.insert( object );
                    cl.delete( "{b:" + value + "}", "{'':'textIndex17945'}" );

                    cl.insert( object );
                    cl.delete( "{b:" + value + "}", "{'':'textIndex17945'}" );

                    cl.insert( object );
                    cl.delete( "{b:" + value + "}", "{'':'textIndex17945'}" );

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
                int endCount = 30;
                while ( true ) {

                    // 开启查询事务
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    TransUtils.queryAndCheck( cl, "{b:1}", "{'':null}",
                            expList );
                    TransUtils.queryAndCheck( cl, "{b:-1}",
                            "{'':'textIndex17945'}", expList2 );

                    // 提交查询事务
                    db.commit();
                    if ( count == endCount ) {
                        break;
                    } else {
                        count++;
                    }
                    Thread.sleep( 150 );
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
