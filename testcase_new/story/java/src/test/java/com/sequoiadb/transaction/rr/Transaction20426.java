package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.Reporter;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20426： 只读事务与更新事务并发，更新的记录overflow，不同时刻发起事务读，隔离级别为RR
 * @date 2020-01-15
 * @author zhaoxiaoni
 */
@Test(groups = "rr")
public class Transaction20426 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private String clName = "cl_20426";
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeMethod
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        db1 = CommLib.getRandomSequoiadb();
        db2 = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "index_20426", "{ a: 1 }", false, false );

        expList.addAll( insertDatas( cl, 0, 10, 128 ) );
        TransUtils.beginTransaction( sdb );
        expList.addAll( insertDatas( cl, 10, 20, 128 ) );
        TransUtils.commitTransaction( sdb );
    }

    @DataProvider(name = "index")
    public Object[][] useIndex() {
        return new Object[][] { { "{ \"\": \"index_20426A\" }" },
                { "{ \"\": null }" } };
    }

    // SEQUOIADBMAINSTREAM-5975
    @Test(dataProvider = "index")
    public void test( String hint ) throws InterruptedException {
        // 开启查询事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 不同coord开启事务需要sleep 100ms
        Thread.sleep( 100 );

        // 开启3个并发事务
        UpdateQueryThread updateThread1 = new UpdateQueryThread( cl1, 64,
                hint );
        updateThread1.start();
        UpdateQueryThread updateThread2 = new UpdateQueryThread( cl2, 256,
                hint );
        updateThread2.start();

        // 判断事务是返回成功
        Assert.assertTrue( updateThread1.isSuccess(),
                updateThread1.getErrorMsg() );
        Assert.assertTrue( updateThread2.isSuccess(),
                updateThread2.getErrorMsg() );
    }

    @AfterMethod
    public void tearDown() {
        // 提交查询事务
        TransUtils.commitTransaction( db1 );
        db1.close();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
        expList.clear();
    }

    private List< BSONObject > insertDatas( DBCollection cl, int start, int end,
            int aLength ) {
        List< BSONObject > records = new ArrayList<>();
        for ( int i = start; i < end; i++ ) {
            String aValue = getRandomString( aLength );
            BSONObject object = ( BSONObject ) JSON.parse(
                    "{ _id: " + i + ", a: '" + aValue + "', b: " + i + " }" );
            records.add( object );
        }
        cl.insert( records );
        return records;
    }

    private String getRandomString( int length ) {
        String str = "abcdefjhijklmnopqistuvwxyzABCDEFJHIJKLMNOPQISTUVWXYZ1234567890";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length; i++ ) {
            int number = random.nextInt( str.length() );
            sb.append( str.charAt( number ) );
        }
        return sb.toString();
    }

    class UpdateQueryThread extends SdbThreadBase {
        private int aLength;
        private String hint;
        private DBCollection queryCL;
        private Sequoiadb db = null;
        private DBCollection cl = null;

        public UpdateQueryThread( DBCollection queryCL, int aLength,
                String hint ) {
            this.aLength = aLength;
            this.hint = hint;
            this.queryCL = queryCL;
        }

        @Override
        public void exec() throws Exception {
            db = CommLib.getRandomSequoiadb();
            cl = db.getCollectionSpace( csName ).getCollection( clName );

            try {
                int doTimes = 1;
                int timeOut = 100;
                while ( true ) {
                    // 开启更新事务
                    TransUtils.beginTransaction( db );

                    String aValue = getRandomString( aLength );
                    int num = new Random().nextInt( expList.size() );
                    System.out.println( Reporter.getCurrentTestResult()
                            .getTestClass().getName() + " update num:" + num );
                    cl.update( "{ 'b': " + num + "}",
                            "{ '$set': { a: '" + aValue + "' } }", hint );

                    // 提交更新事务
                    if ( doTimes % 2 == 0 ) {
                        TransUtils.commitTransaction( db );
                    } else {
                        db.rollback();
                    }

                    // 查询并比较结果
                    TransUtils.queryAndCheck( queryCL, "{_id:1}", hint,
                            expList );
                    if ( doTimes == timeOut ) {
                        break;
                    } else {
                        doTimes++;
                    }
                }
            } finally {
                TransUtils.commitTransaction( db );
                db.close();
            }
        }
    }

}
