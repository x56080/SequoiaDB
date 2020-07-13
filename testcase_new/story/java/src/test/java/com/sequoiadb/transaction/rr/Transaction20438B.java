package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20438: 只读事务与只写事务并发，穿插执行非事务的curd操作，事务读隔离级别为RR
 * @date 2020-01-15
 * @author zhaoxiaoni
 */
@Test(groups = "rr")
public class Transaction20438B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private String clName = "cl_20438B";
    private String idxName = "index_20438";
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
        cl.createIndex( idxName, "{ a: 1 }", false, false );

        // 1.分别在事务中及非事务中插入记录，为R1s
        expList.addAll( TransUtils.insertRandomDatas( cl, 0, 50 ) );// 插入记录为0-50
        TransUtils.beginTransaction( sdb );
        expList.addAll( TransUtils.insertRandomDatas( cl, 50, 100 ) );// 插入记录为50-100
        TransUtils.commitTransaction( sdb );
    }

    @DataProvider(name = "index")
    public Object[][] useIndex() {
        return new Object[][] { { "{ \"\": \"index_20438B\" }" },
                { "{ \"\": null }" } };
    }

    @Test(dataProvider = "index")
    public void test( String hint ) {
        // 开启读事务TR1
        TransUtils.beginTransaction( db1 );
        TransUtils.queryAndCheck( cl1, "{a:1}", hint, expList );

        // 开启写事务TW1，更新R1s为R2s,提交事务，循环执行多次
        for ( int i = 0; i < 3; i++ ) {
            TransUtils.beginTransaction( db2 );
            cl2.update( null, "{$inc:{a:1}}", hint );
            db2.rollback();
        }

        // 事务1查询
        TransUtils.queryAndCheck( cl1, "{a:1}", hint, expList );

        // 非事务更新R2s为R3s
        cl.update( null, "{ '$inc': { 'a': 1 } }", hint );

        // TR1读，检查结果
        expList.clear();
        expList = TransUtils.getIncDatas( 0, 100, 1 );
        TransUtils.queryAndCheck( cl1, "{a:1}", hint, expList );

        // 开启写事务TW2，更新R3s为R4s，提交事务，循环执行多次
        for ( int i = 0; i < 3; i++ ) {
            TransUtils.beginTransaction( db2 );
            cl2.update( null, "{$inc:{a:1}}", hint );
            db2.rollback();
        }

        // TR1读，检查结果
        TransUtils.queryAndCheck( cl1, "{a:1}", hint, expList );

        // 非事务删除记录R4s
        cl.delete( "" );

        // TR1读，检查结果
        expList = TransUtils.deleteList( expList, 0, 100 );

        // TR1读，检查结果
        TransUtils.queryAndCheck( cl1, "{a:1}", hint, expList );

        // 事务中插入记录
        TransUtils.beginTransaction( db2 );
        TransUtils.insertRandomDatas( cl2, 0, 100 );
        TransUtils.commitTransaction( db2 );

        // 查询结果
        TransUtils.queryAndCheck( cl1, "{a:1}", hint, expList );
    }

    @AfterMethod
    public void tearDown() {
        // 提交事务
        TransUtils.commitTransaction( db1 );
        db1.close();
        TransUtils.commitTransaction( db2 );
        db1.close();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
        expList.clear();
    }

}
