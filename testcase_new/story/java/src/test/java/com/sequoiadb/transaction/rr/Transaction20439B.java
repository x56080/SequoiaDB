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
 * @testcase seqDB-20439: 只读事务与只写事务并发，穿插执行非事务的truncate操作，事务读隔离级别为RR
 * @date 2020-01-15
 * @author zhaoxiaoni
 */
@Test(groups = "rr")
public class Transaction20439B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private String clName = "cl_20439B";
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeMethod
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        db1 = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "index_20439B", "{ a: 1 }", false, false );

        // 1.分别在事务中及非事务中插入记录，为R1s
        expList.addAll( TransUtils.insertRandomDatas( cl, 0, 50 ) );// 插入记录为0-50
        TransUtils.beginTransaction( sdb );
        expList.addAll( TransUtils.insertRandomDatas( cl, 50, 100 ) );// 插入记录为50-100
        TransUtils.commitTransaction( sdb );
        // 随机取coord，休眠0.1s，避免从别的coord发起的事务早于上一个事务
        Thread.sleep( 100 );

    }

    @DataProvider(name = "index")
    public Object[][] useIndex() {
        return new Object[][] { { "{ \"\": \"index_20439B\" }" },
                { "{ \"\": null }" } };
    }

    @Test(dataProvider = "index")
    public void test( String hint ) {
        // 2.开启读事务TR1
        // 4.过程中TR1反复读，检查结果
        TransUtils.beginTransaction( db1 );
        QueryThread queryThread = new QueryThread( hint, expList );
        queryThread.start();

        // 3.开启写事务TW1，更新R1s为R2s,提交事务，循环执行多次
        OperatorThread operatorThread = new OperatorThread( hint );
        operatorThread.start();

        // 5.非事务执行truncate操作
        Assert.assertTrue( queryThread.isSuccess(), queryThread.getErrorMsg() );
        Assert.assertTrue( operatorThread.isSuccess(),
                operatorThread.getErrorMsg() );
        cl.truncate();

        // 6.TR1读，检查结果
        // 8.过程中TR1反复读，检查结果
        expList.clear();
        queryThread = new QueryThread( hint, expList );
        queryThread.start();

        Assert.assertTrue( queryThread.isSuccess(), queryThread.getErrorMsg() );
    }

    @AfterMethod
    public void tearDown() {
        // 提交读事务
        TransUtils.commitTransaction( db1 );
        db1.close();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
        expList.clear();
    }

    class OperatorThread extends SdbThreadBase {
        private String hint;
        private Sequoiadb db;
        private DBCollection cl;

        public OperatorThread( String hint ) {
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            db = CommLib.getRandomSequoiadb();
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            try {
                int doTimes = 1;
                int timeOut = 30;
                while ( true ) {
                    // 开启更新事务
                    TransUtils.beginTransaction( db );

                    cl.update(
                            null, "{ '$set': { 'a': "
                                    + ( int ) Math.random() * 100 + "} }",
                            hint );

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
        private String hint;
        private List< BSONObject > expList = new ArrayList<>();

        public QueryThread( String hint, List< BSONObject > expList ) {
            // TODO Auto-generated constructor stub
            this.hint = hint;
            this.expList = expList;
        }

        @Override
        public void exec() throws Exception {
            int doTimes = 1;
            int timeOut = 50;
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
