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
 * @FileName seqDB-17119 : 事务中插入记录与已提交记录唯一索引重复
 * @Author luweikang
 * @Date 2019年1月15日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction17119 extends SdbTestBase {

    private String clName = "transCL_17119";
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
        cl.createIndex( "a", "{a:1}", true, false );
        expDataList = new ArrayList< BSONObject >();

        data = new BasicBSONObject();
        data.put( "a", 1 );
        data.put( "b", "testTrans_17119" );
        data.put( "c", 13700000000L );
        data.put( "d", "customer transaction type data application." );
        cl.insert( data );
        expDataList.add( data );
    }

    @Test
    public void test() {
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            db.beginTransaction();
            DBCollection cl2 = db.getCollectionSpace( csName )
                    .getCollection( clName );

            // 1 trans1 insert already exist record
            cl2.insert( data );
            Assert.fail(
                    "insert an existing record with an index,should be failed" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        } finally {
            db.commit();
            if ( db != null ) {
                db.close();
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
