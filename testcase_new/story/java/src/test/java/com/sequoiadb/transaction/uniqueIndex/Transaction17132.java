package com.sequoiadb.transaction.uniqueIndex;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description Transaction17132.java 更新已提交记录与其他事务更新的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction17132 extends SdbTestBase {

    private String clName = "transCL_17132";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private BSONObject data = null;
    private BSONObject data2 = null;
    private BSONObject data3 = null;
    private BSONObject modifier = null;
    private DBCursor recordCur = null;
    private List< BSONObject > expDataList = null;
    private List< BSONObject > actDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", true, false );
        expDataList = new ArrayList< BSONObject >();

        data = new BasicBSONObject();
        data.put( "_id", "insert1713201" );
        data.put( "a", 1 );
        data.put( "b", "testTrans_17132" );
        data.put( "c", 13700000000L );
        data.put( "d", "customer transaction type data application." );
        cl.insert( data );

        data2 = new BasicBSONObject();
        data2.put( "_id", "insert1713202" );
        data2.put( "a", 2 );
        data2.put( "b", 1024 );
        data2.put( "c", 13700000000L );
        data2.put( "d", "customer transaction type data application." );
        cl.insert( data2 );

        data3 = new BasicBSONObject();
        data3.put( "_id", "id17132" );
        data3.put( "a", 3 );
        data3.put( "b", 2048 );
        data3.put( "c", 13700000000L );
        data3.put( "d", "customer transaction type data application." );
        modifier = new BasicBSONObject();
        modifier.put( "$set", data3 );

        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
    }

    @Test
    public void test1() {

        // trans1 update record R1 to R3
        sdb.beginTransaction();
        cl.update( new BasicBSONObject( "a", 1 ), modifier, null );

        try {
            // trans2 update record R2 to R4 same as the R3
            sdb2.beginTransaction();
            cl2.update( new BasicBSONObject( "a", 2 ), modifier, null );
            Assert.fail(
                    "insert an existing record with an index,should be failed" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        }

        sdb.rollback();
        expDataList.clear();
        expDataList.add( data );
        expDataList.add( data2 );

        recordCur = cl.query( null, null, null, "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl.query( null, null, null, "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

    }

    @Test
    public void test2() {

        // trans1 update record R1 to R3
        sdb.beginTransaction();
        cl.update( new BasicBSONObject( "a", 1 ), modifier, null );

        try {
            // trans2 update record R2 to R4 same as the R3
            sdb2.beginTransaction();
            cl2.update( new BasicBSONObject( "a", 2 ), modifier, null );
            Assert.fail(
                    "insert an existing record with an index,should be failed" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        }

        sdb.commit();
        expDataList.clear();
        expDataList.add( data2 );
        expDataList.add( data3 );

        recordCur = cl.query( null, null, "{'a':1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl.query( null, null, "{'a':1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        cl.delete( "{'a': {'$isnull' :0}}" );
        Assert.assertEquals( cl.getCount(), 0 );

    }

    @AfterClass
    public void tearDown() {
        sdb.commit();
        sdb2.commit();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( recordCur != null ) {
            recordCur.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
        if ( sdb2 != null ) {
            sdb2.close();
        }
    }

}
