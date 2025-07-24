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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20443: 只读事务与只写事务并发，只写事务覆盖：插入、更新、删除，内置SQL隔离级别为RR
 * @date 2020-01-15
 * @author zhaoxiaoni
 */
@Test(groups = "rr")
public class Transaction20443A extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private String clName = "cl_20443A";
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private List< BSONObject > actList = new ArrayList<>();
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeMethod
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        db1 = CommLib.getRandomSequoiadb();
        db2 = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "index_20443A", "{ a: 1 }", false, false );

        expList.addAll( TransUtils.insertRandomDatas( cl, 0, 50 ) );
        TransUtils.beginTransaction( sdb );
        expList.addAll( TransUtils.insertRandomDatas( cl, 50, 100 ) );
        TransUtils.commitTransaction( sdb );
    }

    @DataProvider(name = "index")
    public Object[][] useIndex() {
        return new Object[][] { { "{ \"\": \"index_20443\" }" },
                { "{ \"\": null }" } };
    }

    @Test(dataProvider = "index")
    public void test( String hint ) {
        // 开启读事务TR1
        TransUtils.beginTransaction( db1 );

        // TR1使用内置SQL读记录
        String sql = "select * from " + csName + "." + clName
                + " order by _id /*+use_index(index_20443A)*/";
        DBCursor cursor = db1.exec( sql );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        actList.clear();
        sql = "select * from " + csName + "." + clName
                + " order by _id /*+use_index(NULL)*/";
        cursor = db1.exec( sql );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 2.开启事务TW1，插入记录R2s,提交
        TransUtils.beginTransaction( db2 );
        TransUtils.insertRandomDatas( cl2, 100, 200 );
        TransUtils.commitTransaction( db2 );

        // TR1使用内置SQL读记录
        actList.clear();
        sql = "select * from " + csName + "." + clName
                + " order by _id /*+use_index(index_20443A)*/";
        cursor = db1.exec( sql );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        actList.clear();
        sql = "select * from " + csName + "." + clName
                + " order by _id /*+use_index(NULL)*/";
        cursor = db1.exec( sql );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 3.开启事务TW2，更新记录R2s为R3s，提交
        TransUtils.beginTransaction( db2 );
        cl2.update(
                "{ '$and': [ { '_id': { '$gte': 100 } }, { '_id': { '$lt': 200 } } ] }",
                "{ '$set': { 'a': 'a'} }", hint );
        TransUtils.commitTransaction( db2 );

        // TR1使用内置SQL读记录
        actList.clear();
        sql = "select * from " + csName + "." + clName
                + " order by _id /*+use_index(index_20443A)*/";
        cursor = db1.exec( sql );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        actList.clear();
        sql = "select * from " + csName + "." + clName
                + " order by _id /*+use_index(NULL)*/";
        cursor = db1.exec( sql );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 4.开启事务TW3,删除记录R3s,提交
        TransUtils.beginTransaction( db2 );
        cl2.delete(
                "{ '$and': [ { '_id': { '$gte': 100 } }, { '_id': { '$lt': 200 } } ] }",
                "{ a: 'a'}" );
        TransUtils.commitTransaction( db2 );

        // TR1使用内置SQL读记录
        actList.clear();
        sql = "select * from " + csName + "." + clName
                + " order by _id /*+use_index(index_20443A)*/";
        cursor = db1.exec( sql );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        actList.clear();
        sql = "select * from " + csName + "." + clName
                + " order by _id /*+use_index(NULL)*/";
        cursor = db1.exec( sql );
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
        actList.clear();
        expList.clear();
    }
}
