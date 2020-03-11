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
 * @testcase seqDB-20427 ： 只读事务与删除事务并发，删除的记录overflow，不同时刻发起事务读，隔离级别为RR
 * @date 2020-01-15
 * @author zhaoxiaoni
 */
@Test(groups = "rr")
public class Transaction20427B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private String clName = "cl_20427B";
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();

    @BeforeMethod
    public void setUp() {
        sdb = CommLib.getRandomSequoiadb();
        db1 = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "index_20427B", "{ a: 1 }", false, false );

        expList.addAll( insertDatas( cl, 0, 100, 128 ) );
        sdb.beginTransaction();
        expList.addAll( insertDatas( cl, 100, 200, 128 ) );
        sdb.commit();
    }

    @DataProvider(name = "index")
    public Object[][] useIndex() {
        return new Object[][] { { "{ \"\": \"index_20427B\" }" },
                { "{ \"\": null }" } };
    }

    @Test(dataProvider = "index")
    public void test( String hint ) {
        // 开启查询事务
        db1.beginTransaction();

        // 开启3个并发事务
        QueryThread queryThread = new QueryThread( hint );
        queryThread.start();
        UpdateThread updateThread = new UpdateThread( 256, hint );
        updateThread.start();
        DeleteThread deleteThread = new DeleteThread( hint );
        deleteThread.start();

        // 判断事务是返回成功
        Assert.assertTrue( queryThread.isSuccess(), queryThread.getErrorMsg() );
        Assert.assertTrue( updateThread.isSuccess(),
                updateThread.getErrorMsg() );
        Assert.assertTrue( deleteThread.isSuccess(),
                deleteThread.getErrorMsg() );
    }

    @AfterMethod
    public void tearDown() {
        // 提交读事务
        db1.commit();
        db1.close();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
        expList.clear();
    }

    private List< BSONObject > insertDatas( DBCollection cl, int start, int end,
            int aLength ) {
        List< BSONObject > records = new ArrayList< BSONObject >();
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
        private Sequoiadb db;
        private DBCollection cl;

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
                int timeOut = 100;
                while ( true ) {
                    // 开启更新事务
                    db.beginTransaction();

                    String aValue = getRandomString( aLength );
                    int num = ( int ) ( expList.size() / 2
                            + Math.random() * ( expList.size() / 2 ) );
                    cl.update( "{ 'b': " + num + " }",
                            "{ '$set': { 'a': '" + aValue + "'} }", hint );

                    // 回滚更新事务
                    db.rollback();

                    if ( doTimes == timeOut ) {
                        break;
                    } else {
                        doTimes++;
                    }
                }
            } finally {
                db.rollback();
                db.close();
            }
        }
    }

    class DeleteThread extends SdbThreadBase {
        private String hint;
        private Sequoiadb db;
        private DBCollection cl;

        public DeleteThread( String hint ) {
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            db = CommLib.getRandomSequoiadb();
            cl = db.getCollectionSpace( csName ).getCollection( clName );

            try {
                int doTimes = 1;
                int timeOut = 100;
                while ( true ) {
                    // 开启删除事务
                    db.beginTransaction();

                    int num = ( int ) ( Math.random()
                            * ( expList.size() / 2 ) );
                    cl.delete( "{ 'b': " + num + "}", hint );

                    // 回滚更新事务
                    db.rollback();

                    if ( doTimes == timeOut ) {
                        break;
                    } else {
                        doTimes++;
                    }
                }
            } finally {
                db.rollback();
                db.close();
            }
        }
    }

    class QueryThread extends SdbThreadBase {
        private String hint = null;

        public QueryThread( String hint ) {
            // TODO Auto-generated constructor stub
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            int doTimes = 1;
            int timeOut = 200;
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
