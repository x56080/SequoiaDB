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
 * @Description seqDB-17126 : 插入记录与其他事务中删除的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction17126 extends SdbTestBase {

    private String clName = "transCL_17126";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private BSONObject data = null;
    private BSONObject data2 = null;
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
        data.put( "_id", "id17126" );
        data.put( "a", 1 );
        data.put( "b", "testTrans_17126" );
        data.put( "c", 13700000000L );
        data.put( "d", "customer transaction type data application." );
        cl.insert( data );

        data2 = new BasicBSONObject();
        data2.put( "_id", "id17126" );
        data2.put( "a", 1 );
        data2.put( "b", 1 );
        data2.put( "c", 13700000000L );
        data2.put( "d", "customer transaction type data application." );

        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
    }

    @Test
    public void test1() {

        // 1 trans1 delete R1
        sdb.beginTransaction();
        cl.delete( "{'a': 1}" );

        // 2 trans2 insert record R2 same as the R1
        sdb2.beginTransaction();
        try {
            cl2.insert( data2 );
            Assert.fail(
                    "insert an existing record with an index,should be failed" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        }

        sdb.rollback();

        expDataList.add( data );
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

        // 1 trans1 delete R1
        sdb.beginTransaction();
        cl.delete( "{'a': 1}" );

        // 2 trans2 insert record R2 same as the R1
        sdb2.beginTransaction();
        try {
            cl2.insert( data2 );
            Assert.fail(
                    "insert an existing record with an index,should be failed" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        }

        sdb.commit();

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
