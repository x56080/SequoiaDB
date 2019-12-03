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
 * @Description Transaction17121.java 插入记录与本事务中更新的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction17121 extends SdbTestBase {

    private String clName = "transCL_17121";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private BSONObject data = null;
    private DBCursor recordCur = null;
    private List< BSONObject > expDataList = null;
    private List< BSONObject > actDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        expDataList = new ArrayList< BSONObject >();

        data = new BasicBSONObject();
        data.put( "a", 1 );
        data.put( "b", "testTrans_17121" );
        data.put( "c", 13700000000L );
        data.put( "d", "customer transaction type data application." );
        cl.insert( data );
        expDataList.add( data );
    }

    @Test
    public void test() {

        try {
            sdb.beginTransaction();
            BSONObject data2 = new BasicBSONObject();
            data2.put( "_id", "id17121" );
            data2.put( "a", 17121 );
            data2.put( "b", "testTrans_17121" );
            data2.put( "c", 13700000000L );
            data2.put( "d", "customer transaction type data application." );
            BSONObject modifier = new BasicBSONObject( "$set", data2 );

            // 1 update record R1 to R2
            cl.update( new BasicBSONObject( "a", data.get( "a" ) ), modifier,
                    null );

            // 2 insert record R3 same as the R2
            cl.insert( data2 );
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
