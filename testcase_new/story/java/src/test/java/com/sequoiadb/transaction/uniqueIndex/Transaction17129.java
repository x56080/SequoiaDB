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
 * @Description seqDB-17129 : 更新已提交记录与本事务中更新的记录唯一索引重复
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction17129 extends SdbTestBase {

    private String clName = "transCL_17129";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private BSONObject data = null;
    private BSONObject data2 = null;
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
        data.put( "b", "testTrans_17129" );
        data.put( "c", 13700000000L );
        data.put( "d", "customer transaction type data application." );

        data2 = new BasicBSONObject();
        data2.put( "a", 2 );
        data2.put( "b", 1024 );
        data2.put( "c", 13700000000L );
        data2.put( "d", "customer transaction type data application." );

        expDataList = new ArrayList< BSONObject >();
        expDataList.add( data );
        expDataList.add( data2 );
        cl.insert( expDataList );
        cl.createIndex( "a", "{a:1}", true, false );

        BSONObject data3 = new BasicBSONObject();
        data3.put( "_id", "id17129" );
        data3.put( "a", "testTrans_17129Update" );
        data3.put( "b", 1 );
        data3.put( "c", 13700000000L );
        data3.put( "d", "customer transaction type data application." );

        modifier = new BasicBSONObject();
        modifier.put( "$set", data3 );

    }

    @Test
    public void test() {
        try {
            // update record R1 to R3
            sdb.beginTransaction();
            cl.update( new BasicBSONObject( "a", 1 ), modifier, null );

            // update record R2 to R4 same as the R3
            cl.update( new BasicBSONObject( "a", 2 ), modifier, null );
            Assert.fail(
                    "insert an existing record with an index,should be failed" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        } finally {
            sdb.commit();
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
