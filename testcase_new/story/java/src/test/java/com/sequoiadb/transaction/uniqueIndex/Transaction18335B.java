package com.sequoiadb.transaction.uniqueIndex;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
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
 * @Description Transaction18335.java 回滚的记录与已提交记录唯一索引重复
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction18335B extends SdbTestBase {

    private String clName = "transCL_18335B";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCursor recordCur = null;
    private List< BSONObject > expDataList = null;
    private List< BSONObject > actDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a: 1}", true, false );
        expDataList = new ArrayList< BSONObject >();

        cl.insert( "{'_id': 1, 'a': 1}" );
        cl.insert( "{'_id': 2, 'a': 2}" );
        cl.insert( "{'_id': 3, 'a': 3}" );
        cl.insert( "{'_id': 4, 'a': 4}" );
        cl.insert( "{'_id': 5, 'a': 5}" );
        cl.insert( "{'_id': 6, 'a': 6}" );
        cl.insert( "{'_id': 7, 'a': 7}" );
        cl.insert( "{'_id': 8, 'a': 8}" );
        cl.insert( "{'_id': 9, 'a': 9}" );
        cl.insert( "{'_id': 10, 'a': 10}" );
    }

    @Test
    public void test() {
        sdb.beginTransaction();

        // 1 update R1 to R11 ~ R20
        cl.update( "{_id: 1}", "{'$set': {'a': 11}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 12}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 13}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 14}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 15}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 16}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 17}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 18}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 19}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 20}}", null );

        // 2 update R2 ~ R10 to R11 ~ R19
        DBCollection cl2 = sdb2.getCollectionSpace( csName )
                .getCollection( clName );
        cl2.update( "{_id: 2}", "{'$set': {'a': 11}}", null );
        cl2.update( "{_id: 3}", "{'$set': {'a': 12}}", null );
        cl2.update( "{_id: 4}", "{'$set': {'a': 13}}", null );
        cl2.update( "{_id: 5}", "{'$set': {'a': 14}}", null );
        cl2.update( "{_id: 6}", "{'$set': {'a': 15}}", null );
        cl2.update( "{_id: 7}", "{'$set': {'a': 16}}", null );
        cl2.update( "{_id: 8}", "{'$set': {'a': 17}}", null );
        cl2.update( "{_id: 9}", "{'$set': {'a': 18}}", null );
        cl2.update( "{_id: 10}", "{'$set': {'a': 19}}", null );

        sdb.rollback();

        expDataList.clear();
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 1, 'a': 1}" ) );
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 2, 'a': 11}" ) );
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 3, 'a': 12}" ) );
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 4, 'a': 13}" ) );
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 5, 'a': 14}" ) );
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 6, 'a': 15}" ) );
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 7, 'a': 16}" ) );
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 8, 'a': 17}" ) );
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 9, 'a': 18}" ) );
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 10, 'a': 19}" ) );
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
        sdb.commit();

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
