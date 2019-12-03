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
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-17130 : 更新已提交记录与本事务中删除的记录唯一索引重复
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction17130 extends SdbTestBase {

    private String clName = "transCL_17130";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
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
        data = new BasicBSONObject();
        data.put( "a", 1 );
        data.put( "b", "testTrans_17130" );
        data.put( "c", 13700000000L );
        data.put( "d", "customer transaction type data application." );

        data2 = new BasicBSONObject();
        data2.put( "_id", "id17130" );
        data2.put( "a", 2 );
        data2.put( "b", 1024 );
        data2.put( "c", 13700000000L );
        data2.put( "d", "customer transaction type data application." );

        expDataList = new ArrayList< BSONObject >();
        expDataList.add( data );
        expDataList.add( data2 );
        cl.insert( expDataList );
        cl.createIndex( "a", "{a:1}", true, false );

        data3 = new BasicBSONObject();
        data3.put( "_id", "id17130" );
        data3.put( "a", 1 );
        data3.put( "b", "1024_update" );
        data3.put( "c", 13700000000L );
        data3.put( "d", "customer transaction type data application." );

        modifier = new BasicBSONObject();
        modifier.put( "$set", data3 );

    }

    @Test
    public void test1() {
        Sequoiadb transDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            transDB.beginTransaction();
            DBCollection transCL = transDB.getCollectionSpace( csName )
                    .getCollection( clName );
            // delete record R2
            transCL.delete( new BasicBSONObject( "a", data2.get( "a" ) ) );
            // update record R1 to R3 same as the R2
            transCL.update( new BasicBSONObject( "a", 1 ), modifier, null );

            expDataList.clear();
            expDataList.add( data3 );

            recordCur = cl.query( null, null, null, "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expDataList );
            actDataList.clear();

            recordCur = cl.query( null, null, null, "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expDataList );
            actDataList.clear();

            transDB.rollback();
        } finally {
            transDB.commit();
            if ( transDB != null ) {
                transDB.close();
            }
        }

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
        Sequoiadb transDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            transDB.beginTransaction();
            DBCollection transCL = transDB.getCollectionSpace( csName )
                    .getCollection( clName );
            // delete record R2
            transCL.delete( new BasicBSONObject( "a", data2.get( "a" ) ) );
            // update record R1 to R3 same as the R2
            transCL.update( new BasicBSONObject( "a", 1 ), modifier, null );

            expDataList.clear();
            expDataList.add( data3 );

            recordCur = cl.query( null, null, null, "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expDataList );
            actDataList.clear();

            recordCur = cl.query( null, null, null, "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expDataList );
            actDataList.clear();

            transDB.commit();
        } finally {
            transDB.commit();
            if ( transDB != null ) {
                transDB.close();
            }
        }

        recordCur = cl.query( null, null, null, "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl.query( null, null, null, "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        cl.delete( "{'a': {'$isnull' :0}}" );
        Assert.assertEquals( cl.getCount(), 0 );

    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( recordCur != null ) {
            recordCur.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

}
