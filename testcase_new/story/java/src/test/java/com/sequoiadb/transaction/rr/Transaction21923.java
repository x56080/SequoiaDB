package com.sequoiadb.transaction.rr;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-21923:老事务不使用新创建的索引
 * @author zhaoyu
 * @date 2020.3.10
 */
@Test(groups = "rr")
public class Transaction21923 extends SdbTestBase {

    private String clName = "transCL_21923";
    private Sequoiadb sdb = null;
    private Sequoiadb T1 = null;
    private Sequoiadb T2 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;

    @BeforeClass
    public void setUp() {
        sdb = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        TransUtils.insertRandomDatas( cl, 0, 100 );
    }

    @Test
    public void test() throws InterruptedException {
        T1 = CommLib.getRandomSequoiadb();
        T2 = CommLib.getRandomSequoiadb();

        cl1 = T1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = T2.getCollectionSpace( csName ).getCollection( clName );

        try {
            // 开启事务T1
            TransUtils.beginTransaction( T1 );

            // 创建索引
            cl.createIndex( "index21923", "{a:1}", false, false );

            // 创建索引的过程是同步的，不需要sleep，但是全局事务必须要考虑节点之间的时间差，因此，需要一个sleep时间
            Thread.sleep( 100 );

            // 开启事务T2
            TransUtils.beginTransaction( T2 );

            // T1执行查询，走表扫描
            checkAccessPlan( cl1, 1, "tbscan" );

            // T2执行查询，走索引扫描
            checkAccessPlan( cl2, 1, "ixscan" );

        } finally {
            TransUtils.commitTransaction( T1 );
            TransUtils.commitTransaction( T2 );
        }

    }

    @AfterClass
    public void tearDown() {
        if ( T1 != null ) {
            T1.close();
        }
        if ( T2 != null ) {
            T2.close();
        }
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private void checkAccessPlan( DBCollection cl, int expectRecordNum,
            String expectcanType ) {
        BSONObject matcher = ( BSONObject ) JSON.parse( "{a:10}" );
        BSONObject options = ( BSONObject ) JSON.parse( "{Run:true}" );
        DBCursor cursor = cl.explain( matcher, null, null, null, 0, -1, 0,
                options );
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();

            // 比较记录数
            int returnNum = ( int ) record.get( "ReturnNum" );
            Assert.assertEquals( returnNum, expectRecordNum );

            // 比较是否使用索引扫描
            String scanType = ( String ) record.get( "ScanType" );
            Assert.assertEquals( scanType, expectcanType );
        }
    }

}
