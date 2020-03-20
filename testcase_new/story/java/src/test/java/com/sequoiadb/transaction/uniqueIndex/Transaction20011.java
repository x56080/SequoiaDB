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
 * @FileName seqDB-20011 : 回滚多条记录，回滚第一条插入的记录与过程中插入的记录重复
 * @Author zhaoyu
 * @Date 2019年10月12日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction20011 extends SdbTestBase {

    private String clName = "transCL_20011";
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

    // SEQUOIADBMAINSTREAM-5361
    @Test(enabled = false)
    public void test() {
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        BasicBSONObject r1 = new BasicBSONObject();
        r1.put( "a", 1 );

        BasicBSONObject r2 = new BasicBSONObject();
        r2.put( "a", 2 );
        try {
            db.beginTransaction();
            DBCollection cl1 = db.getCollectionSpace( csName )
                    .getCollection( clName );
            cl1.insert( r1 );
            cl1.insert( r2 );
            cl1.delete( r1 );
            expDataList.add( r2 );
            recordCur = cl1.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expDataList );
            actDataList.clear();
            expDataList.clear();

            cl.insert( r1 );
            expDataList.add( r1 );
            expDataList.add( r2 );
            recordCur = cl1.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expDataList );
            actDataList.clear();
            expDataList.clear();

            db.rollback();
            expDataList.add( r1 );
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
