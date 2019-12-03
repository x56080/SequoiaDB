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
 * @FileName seqDB-20013 : 回滚多条记录，回滚第一条更新的记录与过程中插入的记录重复
 * @Author zhaoyu
 * @Date 2019年10月12日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction20013 extends SdbTestBase {

    private String clName = "transCL_20013";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private DBCursor recordCur = null;
    private List< BSONObject > expDataList = null;
    private List< BSONObject > actDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", true, false );
        expDataList = new ArrayList< BSONObject >();
    }

    @Test
    public void test() {
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        String r1 = "{_id:1,a:1}";
        String r2 = "{_id:2,a:2}";
        String r3 = "{_id:1,a:3}";
        String r4 = "{_id:3,a:1}";
        try {
            db.beginTransaction();
            DBCollection cl1 = db.getCollectionSpace( csName )
                    .getCollection( clName );
            cl1.insert( r1 );
            cl1.insert( r2 );
            cl1.update( "{a:1}", "{$set:{a:3}}", null );
            expDataList.add( ( BSONObject ) JSON.parse( r2 ) );
            expDataList.add( ( BSONObject ) JSON.parse( r3 ) );
            recordCur = cl1.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expDataList );
            actDataList.clear();
            expDataList.clear();

            cl.insert( r4 );
            expDataList.add( ( BSONObject ) JSON.parse( r4 ) );
            expDataList.add( ( BSONObject ) JSON.parse( r2 ) );
            expDataList.add( ( BSONObject ) JSON.parse( r3 ) );
            recordCur = cl1.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expDataList );
            actDataList.clear();
            expDataList.clear();

            db.rollback();
            expDataList.add( ( BSONObject ) JSON.parse( r4 ) );
            recordCur = cl1.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expDataList );
        } finally {
            db.commit();
            if ( db != null ) {
                db.close();
            }
        }
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
