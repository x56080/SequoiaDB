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
 * @Description Transaction17135.java 回滚的记录与本事务中的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction17135C extends SdbTestBase {

    private String clName = "transCL_17135C";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
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
        expDataList = new ArrayList< BSONObject >();

        data = new BasicBSONObject();
        data.put( "a", 1 );
        data.put( "b", 1 );
        data.put( "c", 13700000000L );
        data.put( "d", "customer transaction type data application." );
        expDataList.add( data );

        data2 = new BasicBSONObject();
        data2.put( "a", 1 );
        data2.put( "b", 2 );
        data2.put( "c", 13700000000L );
        data2.put( "d", "customer transaction type data application." );
        expDataList.add( data2 );
        cl.insert( expDataList );

    }

    @Test
    public void test() {

        sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );

        sdb1.beginTransaction();

        // 1 delete record R1
        cl1.delete( "{b:1}" );

        // 2 create unique index
        TransUtils.createUniIdxErr( cl2, "a", "{a:1}" );

        sdb1.rollback();

        recordCur = cl.query( null, null, null, "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList, "check data" );
        actDataList.clear();

        recordCur = cl.query( null, null, null, "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

    }

    @AfterClass
    public void tearDown() {
        sdb1.commit();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( recordCur != null ) {
            recordCur.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
        if ( sdb1 != null ) {
            sdb1.close();
        }
        if ( sdb2 != null ) {
            sdb2.close();
        }
    }

}
