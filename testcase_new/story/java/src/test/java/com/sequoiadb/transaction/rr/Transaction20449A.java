package com.sequoiadb.transaction.rr;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-20449 只读事务与更新事务并发，更新事务使用upsert操作，不同时刻发起事务读，隔离级别为RR
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20449A extends SdbTestBase {

    private String clName = "transCL_20449A";
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private DBCollection cl = null;
    private DBCollection clTR1 = null;
    private DBCollection clTW1 = null;
    private int recordNum = 100;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        expDataList = TransUtils.prepareDatas( sdb, cl, recordNum );
    }

    @Test
    public void test() throws InterruptedException {
        TR1 = CommLib.getRandomSequoiadb();
        TW1 = CommLib.getRandomSequoiadb();

        clTR1 = TR1.getCollectionSpace( csName ).getCollection( clName );
        clTW1 = TW1.getCollectionSpace( csName ).getCollection( clName );

        // 1 begin trans TR1 read
        TransUtils.beginTransaction( TR1 );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 2 begin trans TW1 upsert R1s to R3s
        TransUtils.beginTransaction( TW1 );
        clTW1.upsert(
                ( BSONObject ) JSON.parse( "{'a': {'$gte': 0, '$lt': 100}}" ),
                ( BSONObject ) JSON.parse(
                        "{'$inc':{a: 1}, '$set': {'b': 'update r1s to r3s'}}" ),
                ( BSONObject ) JSON.parse( "{'': 'a'}" ) );
        clTW1.upsert(
                ( BSONObject ) JSON.parse( "{'a': {'$gte': 200, '$lt': 300}}" ),
                ( BSONObject ) JSON.parse(
                        "{'$inc':{a: 1}, '$set': {'b': 'update r2s to r4s'}}" ),
                ( BSONObject ) JSON.parse( "{'': 'a'}" ) );
        TransUtils.commitTransaction( TW1 );

        // 1 trans TR1 read
        TransUtils.beginTransaction( TR1 );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        TransUtils.commitTransaction( TR1 );
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
}
