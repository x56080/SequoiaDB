package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20440: 只读事务与只写事务并发，在多个集合空间下的多个集合执行事务操作，隔离级别为RR
 * @date 2020-01-15
 * @author zhaoxiaoni
 */
@Test(groups = "rr")
public class Transaction20440B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private Sequoiadb TR2 = null;
    private Sequoiadb TW2 = null;
    private Sequoiadb TR3 = null;
    private Sequoiadb TR4 = null;
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

        for ( int i = 0; i < 2; i++ ) {
            CollectionSpace cs = sdb.createCollectionSpace( "cs_20440B_" + i );
            for ( int j = 0; j < 2; j++ ) {
                DBCollection cl = cs.createCollection( "cl_20440B_" + j );
                cl.createIndex( "index_20440B", "{ a: 1 }", false, false );

                // 1.分别在事务中及非事务中插入记录，R1s+R2s+R3s
                TransUtils.insertRandomDatas( cl, 0, 100 );// 插入记录为0-100
                TransUtils.beginTransaction( sdb );
                TransUtils.insertRandomDatas( cl, 100, 200 );// 插入记录为100-200
                TransUtils.commitTransaction( sdb );
                TransUtils.insertRandomDatas( cl, 200, 300 );// 插入记录为200-300
            }
        }

        expList = TransUtils.addList( expList, 0, 300 );
    }

    @DataProvider(name = "index")
    public Object[][] useIndex() {
        return new Object[][] { { "{ \"\": \"index_20440B\" }" },
                { "{ \"\": null }" } };
    }

    @Test(dataProvider = "index")
    public void test( String hint ) {
        // 1.开启读事务TR1,所有事务读记录
        TransUtils.beginTransaction( TR1 );
        QueryThread queryThread1 = new QueryThread( TR1, hint,
                new ArrayList<>( expList ) );
        queryThread1.start();

        // 2.开启写事务TW1，在多个集合下插入记录R4s，更新记录R1s为R5s,删除记录R2s，并回滚
        TransUtils.beginTransaction( TW1 );
        for ( int i = 0; i < 2; i++ ) {
            CollectionSpace cs = TW1.getCollectionSpace( "cs_20440B_" + i );
            for ( int j = 0; j < 2; j++ ) {
                DBCollection cl = cs.getCollection( "cl_20440B_" + j );
                TransUtils.insertRandomDatas( cl, 300, 400 );
                cl.update(
                        "{ '$and': [ { '_id': { '$gte': 0 } }, { '_id': { '$lt': 100 } }] }",
                        "{ '$set': { 'a': 400 } }", hint );
                cl.delete(
                        "{ '$and': [ { '_id': { '$gte': 100 } }, { '_id': { '$lt': 200 } }] }" );
            }
        }
        TW1.rollback();

        // 3.开启读事务TR3，所有读事务读记录,检查结果
        TransUtils.beginTransaction( TR2 );
        QueryThread queryThread2 = new QueryThread( TR2, hint, expList );
        queryThread2.start();

        // 4.开启写事务TW2在多个集合下插入记录R6s,更新记录R5s为R7s,删除记录R3s;
        TransUtils.beginTransaction( TW2 );
        for ( int i = 0; i < 2; i++ ) {
            CollectionSpace cs = TW2.getCollectionSpace( "cs_20440B_" + i );
            for ( int j = 0; j < 2; j++ ) {
                DBCollection cl = cs.getCollection( "cl_20440B_" + j );
                TransUtils.insertRandomDatas( cl, 500, 600 );
                cl.update( "{ 'a': 400 }", "{ '$set': { 'a': 600 } }", hint );
                cl.delete(
                        "{ '$and': [ { '_id': { '$gte': 200 } }, { '_id': { '$lt': 300 } }] }" );
            }
        }

        // 5.开启读事务TR3,所有读事务读记录
        TransUtils.beginTransaction( TR3 );
        QueryThread queryThread3 = new QueryThread( TR3, hint,
                new ArrayList<>( expList ) );
        queryThread3.start();

        // 6.回滚写事务TW2
        TW2.rollback();

        // 7.开启读事务TR4,所有读事务读记录，检查结果
        TransUtils.beginTransaction( TR4 );
        QueryThread queryThread4 = new QueryThread( TR4, hint, expList );
        queryThread4.start();

        Assert.assertTrue( queryThread1.isSuccess(),
                queryThread1.getErrorMsg() );
        Assert.assertTrue( queryThread2.isSuccess(),
                queryThread2.getErrorMsg() );
        Assert.assertTrue( queryThread3.isSuccess(),
                queryThread3.getErrorMsg() );
        Assert.assertTrue( queryThread4.isSuccess(),
                queryThread4.getErrorMsg() );
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

        for ( int i = 0; i < 2; i++ ) {
            sdb.dropCollectionSpace( "cs_20440B_" + i );
        }
        sdb.close();
        expList.clear();
    }

    class QueryThread extends SdbThreadBase {
        private Sequoiadb db = null;
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
            int doTimes = 1;
            int timeOut = 50;
            while ( true ) {
                for ( int i = 0; i < 2; i++ ) {
                    CollectionSpace cs = db
                            .getCollectionSpace( "cs_20440B_" + i );
                    for ( int j = 0; j < 2; j++ ) {
                        DBCollection cl = cs.getCollection( "cl_20440B_" + j );
                        TransUtils.checkRecord( cl, null, null, "{ _id: 1}",
                                hint, expList );
                    }
                }
                if ( doTimes == timeOut ) {
                    break;
                } else {
                    doTimes++;
                }
            }
        }
    }
}
