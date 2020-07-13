package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
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
 * @testcase seqDB-21849:只读事务与更新事务并发，更新的记录为非事务中已overflow的记录，不同时刻发起事务读，隔离级别为RR
 * @date 2020-02-19
 * @author zhaoxiaoni
 */
@Test(groups = "rr")
public class Transaction21849A extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private String clName = "cl_21849A";
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeMethod
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        db1 = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "index_20426A", "{ a: 1 }", false, false );

        expList.addAll( insertDatas( cl, 0, 50, 128 ) );
        TransUtils.beginTransaction( sdb );
        expList.addAll( insertDatas( cl, 50, 100, 128 ) );
        TransUtils.commitTransaction( sdb );
    }

    @DataProvider(name = "index")
    public Object[][] useIndex() {
        return new Object[][] { { "{ \"\": \"index_20426A\" }" },
                { "{ \"\": null }" } };
    }

    @Test(dataProvider = "index")
    public void test( String hint ) {
        // 1.非事务中更新R1s为R2s，记录overflow
        String aValue = getRandomString( 256 );
        cl.update( null, "{ '$set': { a: '" + aValue + "' } }", hint );
        TransUtils.updateList( expList, 0, 100, aValue );

        // 2.开启读事务TR1，开启写事务TW1,更新记录R2s为R3s，并提交，TR1读记录
        TransUtils.beginTransaction( db1 );
        QueryThread queryThread = new QueryThread( hint );
        queryThread.start();
        UpdateThread updateThread1 = new UpdateThread( 64, hint );
        updateThread1.start();

        // 判断事务是返回成功
        Assert.assertTrue( queryThread.isSuccess(), queryThread.getErrorMsg() );
        Assert.assertTrue( updateThread1.isSuccess(),
                updateThread1.getErrorMsg() );
    }

    @AfterMethod
    public void tearDown() {
        // 提交读事务TR1
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

    class UpdateThread extends SdbThreadBase {
        private int aLength;
        private String hint;
        private Sequoiadb db = null;
        private DBCollection cl = null;

        public UpdateThread( int aLength, String hint ) {
            this.aLength = aLength;
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            db = CommLib.getRandomSequoiadb();
            cl = db.getCollectionSpace( csName ).getCollection( clName );

            try {
                int doTimes = 1;
                int timeOut = 60;
                while ( true ) {
                    // 开启更新事务
                    TransUtils.beginTransaction( db );

                    String aValue = getRandomString( aLength );
                    int num = new Random().nextInt( expList.size() );
                    cl.update( "{ 'b': " + num + "}",
                            "{ '$set': { a: '" + aValue + "' } }", hint );

                    // 提交更新事务
                    TransUtils.commitTransaction( db );
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

    class QueryThread extends SdbThreadBase {
        private String hint = null;

        public QueryThread( String hint ) {
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            int doTimes = 1;
            int timeOut = 300;
            while ( true ) {
                TransUtils.checkRecord( cl1, null, null, "{ _id: 1}", hint,
                        expList );

                if ( doTimes == timeOut ) {
                    break;
                } else {
                    doTimes++;
                }
            }
        }
    }
}
