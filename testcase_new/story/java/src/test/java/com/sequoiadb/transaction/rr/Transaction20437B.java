package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
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
 * @testcase seqDB-20437:
 *           只读事务与只写事务并发，只写事务覆盖：插入、更新、删除，TW1事务包含TW2、TW3、TW4,不同时刻发起事务读，隔离级别为RR
 * @date 2020-01-15
 * @author zhaoxiaoni
 */
@Test(groups = "rr")
public class Transaction20437B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private Sequoiadb TR2 = null;
    private Sequoiadb TW2 = null;
    private Sequoiadb TR3 = null;
    private Sequoiadb TR4 = null;
    private Sequoiadb TW3 = null;
    private Sequoiadb TR5 = null;
    private Sequoiadb TR6 = null;
    private Sequoiadb TW4 = null;
    private Sequoiadb TR7 = null;
    private Sequoiadb TR8 = null;
    private Sequoiadb TR9 = null;
    private Sequoiadb TR10 = null;
    private String clName = "cl_20437B";
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl4 = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeMethod
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        TR1 = CommLib.getRandomSequoiadb();
        TW1 = CommLib.getRandomSequoiadb();
        TR2 = CommLib.getRandomSequoiadb();
        TW2 = CommLib.getRandomSequoiadb();
        TR3 = CommLib.getRandomSequoiadb();
        TR4 = CommLib.getRandomSequoiadb();
        TW3 = CommLib.getRandomSequoiadb();
        TR5 = CommLib.getRandomSequoiadb();
        TR6 = CommLib.getRandomSequoiadb();
        TW4 = CommLib.getRandomSequoiadb();
        TR7 = CommLib.getRandomSequoiadb();
        TR8 = CommLib.getRandomSequoiadb();
        TR9 = CommLib.getRandomSequoiadb();
        TR10 = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = TW1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = TW2.getCollectionSpace( csName ).getCollection( clName );
        cl3 = TW3.getCollectionSpace( csName ).getCollection( clName );
        cl4 = TW4.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "index_20437B", "{ a: 1 }", false, false );

        expList.addAll( TransUtils.insertRandomDatas( cl, 0, 200 ) );// 插入记录为0-200
        TransUtils.beginTransaction( sdb );
        expList.addAll( TransUtils.insertRandomDatas( cl, 200, 400 ) );// 插入记录为200-400
        TransUtils.commitTransaction( sdb );
    }

    @DataProvider(name = "index")
    public Object[][] useIndex() {
        return new Object[][] { { "{ \"\": \"index_20437B\" }" },
                { "{ \"\": null }" } };
    }

    @Test(dataProvider = "index")
    public void test( String hint ) {
        // 1.开启读事务TR1，所有读事务读记录
        TransUtils.beginTransaction( TR1 );
        QueryThread queryThread1 = new QueryThread( TR1, hint,
                new ArrayList<>( expList ) );
        queryThread1.start();

        // 2.开启写事务TW1
        TransUtils.beginTransaction( TW1 );

        // 3.开启读事务TR2，所有读事务读记录
        TransUtils.beginTransaction( TR2 );
        QueryThread queryThread2 = new QueryThread( TR2, hint,
                new ArrayList<>( expList ) );
        queryThread2.start();

        // 4.开启写事务TW2,插入记录R5s，更新记录R1s为R6s，删除记录R2s，开启读事务TR3,所有读事务读记录
        TransUtils.beginTransaction( TW2 );
        TransUtils.insertRandomDatas( cl2, 400, 500 );// 插入记录400-500
        cl2.update( "{ '_id': { '$lt': 100 } }", "{ '$set': { 'a': 600 } }",
                hint );// 将0-100的记录更新成600
        cl2.delete(
                "{ '$and': [ { '_id': { '$gte': 100 } }, { '_id': { '$lt': 200 } }] }" );// 删除100-200记录

        TransUtils.beginTransaction( TR3 );
        QueryThread queryThread3 = new QueryThread( TR3, hint,
                new ArrayList<>( expList ) );
        queryThread3.start();

        // 5.回滚TW2,开启读事务TR4，所有读事务读记录
        TW2.rollback();
        TW2.close();
        TransUtils.beginTransaction( TR4 );
        QueryThread queryThread4 = new QueryThread( TR4, hint, expList );
        queryThread4.start();

        // 6.开启写事务TW3,插入记录R7s，更新R6s为R8s，删除记录R3s，开启读事务TR5,所有读事务读记录
        TransUtils.beginTransaction( TW3 );
        TransUtils.insertRandomDatas( cl3, 600, 700 );// 插入记录600-700
        cl3.update( "{ '_id': { '$lt': 100 } }", "{ '$set': { 'a': 800 } }",
                hint );// 将0-100的记录更新成800
        cl3.delete(
                "{ '$and': [ { '_id': { '$gte': 200 } }, { '_id': { '$lt': 300 } }] }" );// 删除200-300记录

        TransUtils.beginTransaction( TR5 );
        QueryThread queryThread5 = new QueryThread( TR5, hint,
                new ArrayList<>( expList ) );
        queryThread5.start();

        // 7.回滚TW3,开启读事务TR6,所有读事务读记录
        TW3.rollback();
        TW3.close();
        TransUtils.beginTransaction( TR6 );
        QueryThread queryThread6 = new QueryThread( TR6, hint, expList );
        queryThread6.start();

        // 8.开启写事务TW4，插入记录R9s,更新R8s为R10s，删除记录R4s，开启读事务TR7,所有读事务读记录
        TransUtils.beginTransaction( TW4 );
        TransUtils.insertRandomDatas( cl4, 800, 900 );// 插入字段值为800-900的记录
        cl4.update( "{ '_id': { '$lt': 100 } }", "{ '$set': { 'a': 900 } }",
                hint );// 将_id字段值为0-100的记录的a字段值更新成900
        cl4.delete(
                "{ '$and': [ { '_id': { '$gte': 300 } }, { '_id': { '$lt': 400 } }] }" );// 删除_id字段值为300-400的记录

        TransUtils.beginTransaction( TR7 );
        QueryThread queryThread7 = new QueryThread( TR7, hint,
                new ArrayList<>( expList ) );
        queryThread7.start();

        // 9.回滚TW4,开启读事务TR8,所有读事务读记录
        TW4.rollback();
        TW4.close();
        TransUtils.beginTransaction( TR8 );
        QueryThread queryThread8 = new QueryThread( TR8, hint, expList );
        queryThread8.start();

        // 10.TW1插入记录R11s，更新R10s为R12s,删除记录R5s+R7s+R9s,开启读事务TR9,所有读事务读记录
        TransUtils.insertRandomDatas( cl1, 1000, 1100 );
        cl1.update( "{ '_id': { '$lt': 100 } }", "{ '$set': { 'a': 1100 } }",
                hint );
        cl1.delete(
                "{ '$and': [ { '_id': { '$gte': 400 } }, { '_id': { '$lt': 900 } }] }" );

        TransUtils.beginTransaction( TR9 );
        QueryThread queryThread9 = new QueryThread( TR9, hint,
                new ArrayList<>( expList ) );
        queryThread9.start();

        // 11.回滚事务TW1,开启读事务TR10,所有读事务读记录
        TW1.rollback();
        TW1.close();
        TransUtils.beginTransaction( TR10 );
        QueryThread queryThread10 = new QueryThread( TR10, hint, expList );
        queryThread10.start();

        // 判断事务是返回成功
        Assert.assertTrue( queryThread1.isSuccess(),
                queryThread1.getErrorMsg() );
        Assert.assertTrue( queryThread3.isSuccess(),
                queryThread3.getErrorMsg() );
        Assert.assertTrue( queryThread4.isSuccess(),
                queryThread4.getErrorMsg() );
        Assert.assertTrue( queryThread5.isSuccess(),
                queryThread5.getErrorMsg() );
        Assert.assertTrue( queryThread6.isSuccess(),
                queryThread6.getErrorMsg() );
        Assert.assertTrue( queryThread7.isSuccess(),
                queryThread7.getErrorMsg() );
        Assert.assertTrue( queryThread8.isSuccess(),
                queryThread8.getErrorMsg() );
        Assert.assertTrue( queryThread9.isSuccess(),
                queryThread9.getErrorMsg() );
        Assert.assertTrue( queryThread10.isSuccess(),
                queryThread10.getErrorMsg() );
    }

    @AfterMethod
    public void tearDown() {
        TransUtils.commitTransaction( TR1 );
        TR1.close();
        TransUtils.commitTransaction( TR2 );
        TR2.close();
        TransUtils.commitTransaction( TR3 );
        TR3.close();
        TransUtils.commitTransaction( TR4 );
        TR4.close();
        TransUtils.commitTransaction( TR5 );
        TR5.close();
        TransUtils.commitTransaction( TR6 );
        TR6.close();
        TransUtils.commitTransaction( TR7 );
        TR7.close();
        TransUtils.commitTransaction( TR8 );
        TR8.close();
        TransUtils.commitTransaction( TR9 );
        TR9.close();
        TransUtils.commitTransaction( TR10 );
        TR10.close();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
        expList.clear();
    }

    class QueryThread extends SdbThreadBase {
        private Sequoiadb db = null;
        private DBCollection cl = null;
        private String hint = null;
        private List< BSONObject > expList = new ArrayList<>();

        public QueryThread( Sequoiadb db, String hint,
                List< BSONObject > expList ) {
            // TODO Auto-generated constructor stub
            this.db = db;
            this.hint = hint;
            this.expList = expList;
        }

        @Override
        public void exec() throws Exception {
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            int doTimes = 1;
            int timeOut = 50;
            while ( true ) {
                TransUtils.checkRecord( cl, null, null, "{ _id: 1}", hint,
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
