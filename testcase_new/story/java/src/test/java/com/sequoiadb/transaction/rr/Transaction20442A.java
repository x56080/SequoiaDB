package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20442: 只读事务与只写事务并发，只写事务覆盖：插入、更新、删除，aggregate接口隔离级别为RR
 * @date 2020-01-15
 * @author zhaoxiaoni
 */
@Test(groups = "rr")
public class Transaction20442A extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private String clName = "cl_20442A";
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private List< BSONObject > actList = new ArrayList<>();
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeMethod
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        db1 = CommLib.getRandomSequoiadb();
        db2 = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "index_20442A", "{ a: 1 }", false, false );

        expList.addAll( TransUtils.insertRandomDatas( cl, 0, 50 ) );
        TransUtils.beginTransaction( sdb );
        expList.addAll( TransUtils.insertRandomDatas( cl, 50, 100 ) );
        TransUtils.commitTransaction( sdb );
    }

    @DataProvider(name = "index")
    public Object[][] useIndex() {
        return new Object[][] { { "{ \"\": \"index_20442\" }" },
                { "{ \"\": null }" } };
    }

    @Test(dataProvider = "index")
    public void test( String hint ) {
        // 1.开启读事务TR1
        TransUtils.beginTransaction( db1 );

        // TR1使用aggregate接口读记录
        List< BSONObject > objects = new ArrayList<>();
        objects.add( ( BSONObject ) JSON.parse( "{ $sort: { _id: 1 } }" ) );
        DBCursor cursor = cl1.aggregate( objects );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 2.开启事务TW1，插入记录R2s,提交
        TransUtils.beginTransaction( db2 );
        TransUtils.insertRandomDatas( cl2, 100, 200 );
        TransUtils.commitTransaction( db2 );

        // TR1使用aggregate接口读记录
        cursor = cl1.aggregate( objects );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 3.开启事务TW2，更新记录R2s为R3s，提交
        TransUtils.beginTransaction( db2 );
        cl2.update(
                "{ '$and': [ { '_id': { '$gte': 100 } }, { '_id': { '$lt': 200 } } ] }",
                "{ '$set': { 'a': 'a'} }", hint );
        TransUtils.commitTransaction( db2 );

        // TR1使用aggregate接口读记录
        cursor = cl1.aggregate( objects );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 4.开启事务TW3,删除记录R3s,提交
        TransUtils.beginTransaction( db2 );
        cl2.delete(
                "{ '$and': [ { '_id': { '$gte': 100 } }, { '_id': { '$lt': 200 } } ] }",
                "{ a: 'a'}" );
        TransUtils.commitTransaction( db2 );

        // TR1使用aggregate接口读记录
        cursor = cl1.aggregate( objects );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
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

}
