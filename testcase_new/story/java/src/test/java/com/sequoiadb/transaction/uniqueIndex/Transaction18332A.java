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
 * @Description Transaction18332.java 回滚的记录与已提交记录唯一索引重复
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction18332A extends SdbTestBase {

    private String clName = "transCL_18332A";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCursor recordCur = null;
    private List< BSONObject > expDataList = null;
    private List< BSONObject > actDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a: 1}", true, false );
        expDataList = new ArrayList< BSONObject >();

        cl.insert( "{'_id': 1, 'a': 1}" );
        cl.insert( "{'_id': 2, 'a': 2}" );
    }

    @Test
    public void test() {

        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        sdb.beginTransaction();
        sdb2.beginTransaction();

        // 1 update R1 to R3/R4/R5
        cl.update( "{_id: 1}", "{'$set': {'a': 3}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 4}}", null );
        cl.update( "{_id: 1}", "{'$set': {'a': 5}}", null );

        // 2 trans2 update R2 to R6
        DBCollection cl2 = sdb2.getCollectionSpace( csName )
                .getCollection( clName );
        cl2.update( "{_id: 2}", "{'$set': {'a': 3}}", null );

        sdb.rollback();
        sdb2.commit();

        expDataList.clear();
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 1, 'a': 1}" ) );
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 2, 'a': 3}" ) );
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
