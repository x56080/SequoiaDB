package com.sequoiadb.transaction.rr;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
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
 * @Description seqDB-20453:老事务在新建索引后不再生成访问计划缓存
 * @author luweikang
 * @modify zhaoyu
 * @date 2020.3.10
 */
@Test(groups = "rr")
public class Transaction20453 extends SdbTestBase {

    private String clName = "transCL_20453";
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
            // 创建索引
            cl.createIndex( "index20453_1", "{a:1}", false, false );

            // 创建索引的过程是同步的，不需要sleep，但是全局事务必须要考虑节点之间的时间差，因此，需要一个sleep时间
            Thread.sleep( 100 );

            // 开启事务T1
            TransUtils.beginTransaction( T1 );

            // 创建索引
            cl.createIndex( "index20453_2", "{a:1,b:1}", false, false );

            // 创建索引的过程是同步的，不需要sleep，但是全局事务必须要考虑节点之间的时间差，因此，需要一个sleep时间
            Thread.sleep( 100 );

            // 开启事务T2
            TransUtils.beginTransaction( T2 );

            // 执行5次查询生成访问计划缓存
            ArrayList< BSONObject > expList = new ArrayList<>();
            for ( int i = 0; i < 5; i++ ) {
                expList.clear();
                BSONObject record = ( BSONObject ) JSON
                        .parse( "{_id:" + i + ",a:" + i + ",b:" + i + "}" );
                expList.add( record );
                TransUtils.checkQueryResultOnly( cl,
                        "{a:" + i + ",b:" + i + "}", "", "", expList );
            }
            int accessPlanNum = getAccessPlanNum( sdb, csName + "." + clName );
            Assert.assertEquals( accessPlanNum, 1 );

            // T1执行{a:i,b:i}的查询，未命中查询计划缓存
            BSONObject matcher = ( BSONObject ) JSON.parse( "{a:10,b:10}" );
            checkAccessPlan( cl1, matcher, 1, "NoCache" );

            // T2执行{a:i,b:i}的查询，命中查询计划缓存
            checkAccessPlan( cl2, matcher, 1, "HitCache" );

            // T1执行{a:1}的匹配查询，无法再生成新的访问计划缓存
            expList.clear();
            BSONObject record = ( BSONObject ) JSON.parse( "{_id:1,a:1,b:1}" );
            expList.add( record );
            TransUtils.checkQueryResultOnly( cl1, "{a:1}", "", "", expList );
            accessPlanNum = getAccessPlanNum( sdb, csName + "." + clName );
            Assert.assertEquals( accessPlanNum, 1 );

            // T2执行{a:1}的匹配查询，正常生成新的访问计划缓存
            TransUtils.checkQueryResultOnly( cl2, "{a:1}", "", "", expList );
            accessPlanNum = getAccessPlanNum( sdb, csName + "." + clName );
            Assert.assertEquals( accessPlanNum, 2 );

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

    private void checkAccessPlan( DBCollection cl, BSONObject matcher,
            int expectRecordNum, String expectCacheStatus ) {
        BSONObject options = ( BSONObject ) JSON
                .parse( "{Run:true,Detail:true}" );
        DBCursor cursor = cl.explain( matcher, null, null, null, 0, -1, 0,
                options );
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();

            // 比较记录数
            int returnNum = ( int ) record.get( "ReturnNum" );
            Assert.assertEquals( returnNum, expectRecordNum );

            // 比较是否命中查询计划缓存
            BSONObject planPath = ( BSONObject ) record.get( "PlanPath" );
            BasicBSONList childOperators = ( BasicBSONList ) planPath
                    .get( "ChildOperators" );
            BSONObject nodeOperator = ( BSONObject ) childOperators.get( 0 );
            String cacheStatus = ( String ) nodeOperator.get( "CacheStatus" );
            Assert.assertEquals( cacheStatus, expectCacheStatus );
        }
        cursor.close();
    }

    private int getAccessPlanNum( Sequoiadb db, String clFullName ) {
        BSONObject matcher = ( BSONObject ) JSON
                .parse( "{Collection:'" + clFullName + "'}" );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_ACCESSPLANS,
                matcher, null, null );
        int accessPlanNum = 0;
        while ( cursor.hasNext() ) {
            cursor.getNext();
            accessPlanNum++;
        }
        cursor.close();
        return accessPlanNum;
    }

}
