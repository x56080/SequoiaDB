package com.sequoiadb.transaction.rr;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @Description seqDB-20453 已生成访问计划缓存，删除索引，事务读需重新生成访问计划
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20453 extends SdbTestBase {

    private String clName = "transCL_20453";
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private DBCollection cl = null;
    private DBCollection clTR1 = null;
    private DBCollection clTW1 = null;
    private int recordNum = 1000;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        expDataList = TransUtils.prepareDatas( sdb, cl, recordNum );
    }

    @Test
    public void test() throws InterruptedException {
        TR1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        TW1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        clTR1 = TR1.getCollectionSpace( csName ).getCollection( clName );
        clTW1 = TW1.getCollectionSpace( csName ).getCollection( clName );

        // 1 begin trans TR1
        TR1.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{a: {'$gte': 0, '$lt': 1000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{a: {'$gte': 0, '$lt': 1000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        checkAccessPlans( sdb, csName, clName, 2 );

        // 2 begin trans TW1 upsert R1s to R2s
        TW1.beginTransaction();
        clTW1.update( "{'a': {'$gte': 0, '$lt': 1000}}", "{'$inc': {'a': 1}}",
                "{'': 'a'}" );
        TW1.commit();

        // 3 drop index
        cl.dropIndex( "a" );

        // 3 trans TR1 explain
        DBCursor cur = clTR1.explain(
                ( BSONObject ) JSON.parse( "{'a': {'$gte': 0, '$lt': 1000}}" ),
                null, null, new BasicBSONObject( "", "a" ), 0, -1, 0,
                new BasicBSONObject( "Detail", true ) );
        while ( cur.hasNext() ) {
            BSONObject explain = cur.getNext();
            BSONObject planPath = ( BSONObject ) explain.get( "PlanPath" );
            @SuppressWarnings("unchecked")
            List< BSONObject > childOperators = ( List< BSONObject > ) planPath
                    .get( "ChildOperators" );
            Object CacheStatus = childOperators.get( 0 ).get( "CacheStatus" );
            Assert.assertEquals( CacheStatus, "NewCache", explain.toString() );
        }
        cur.close();

        TR1.commit();

    }

    @AfterClass
    public void tearDown() {
        if ( TR1 != null ) {
            TR1.close();
        }
        if ( TW1 != null ) {
            TW1.close();
        }
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private void checkAccessPlans( Sequoiadb db, String csName, String clName,
            int planNum ) {
        String fullNama = csName + "." + clName;
        DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_ACCESSPLANS,
                "{Collection: '" + fullNama + "'}", "", "" );
        int count = 0;
        while ( cur.hasNext() ) {
            cur.getNext();
            count++;
        }
        cur.close();
        Assert.assertEquals( count, planNum, "check cl accessplans num" );
    }
}
