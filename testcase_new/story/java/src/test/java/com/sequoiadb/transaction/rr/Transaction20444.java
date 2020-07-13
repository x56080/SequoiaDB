package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
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
 * @Description seqDB-20444 读事务中使用findAndUpdate，隔离级别验证
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20444 extends SdbTestBase {

    private String clName = "transCL_20444";
    private Sequoiadb sdb = null;
    private Sequoiadb T1 = null;
    private Sequoiadb T2 = null;
    private Sequoiadb T3 = null;
    private Sequoiadb T4 = null;
    private DBCollection cl = null;
    private DBCollection clT1 = null;
    private DBCollection clT2 = null;
    private DBCollection clT3 = null;
    private DBCollection clT4 = null;
    private int recordNum = 300;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        expDataList = TransUtils.prepareDatas( sdb, cl, recordNum );
    }

    // SEQUOIADBMAINSTREAM-5589
    @Test
    public void test() throws InterruptedException {
        T1 = CommLib.getRandomSequoiadb();
        T2 = CommLib.getRandomSequoiadb();
        T3 = CommLib.getRandomSequoiadb();
        T4 = CommLib.getRandomSequoiadb();

        clT1 = T1.getCollectionSpace( csName ).getCollection( clName );
        clT2 = T2.getCollectionSpace( csName ).getCollection( clName );
        clT3 = T3.getCollectionSpace( csName ).getCollection( clName );
        clT4 = T4.getCollectionSpace( csName ).getCollection( clName );

        // 1 begin trans T1 read
        TransUtils.beginTransaction( T1 );
        TransUtils.queryAndCheck( clT1, "{'a': {$gte:0, $lt: 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clT1, "{'a': {$gte:0, $lt: 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 2 begin trans T2 update R1s to R4s
        TransUtils.beginTransaction( T2 );
        clT2.update( "{'a': {'$gte': 0, '$lt': 100}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r1s to r4s'}}",
                "{'': 'a'}" );
        TransUtils.commitTransaction( T2 );

        // 3 begin trans T3 update R2s to R5s
        TransUtils.beginTransaction( T3 );
        clT3.update( "{'a': {'$gte': 100, '$lt': 200}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r2s to r5s'}}",
                "{'': 'a'}" );
        TransUtils.commitTransaction( T3 );

        // 4 begin trans T4 update R3s to R6s
        TransUtils.beginTransaction( T4 );
        clT4.update( "{'a': {'$gte': 200, '$lt': 300}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r3s to r6s'}}",
                "{'': 'a'}" );
        TransUtils.commitTransaction( T4 );

        // T1 read the records
        TransUtils.queryAndCheck( clT1, "{'a': {$gte:0, $lt: 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clT1, "{'a': {$gte:0, $lt: 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        DBCursor cur = clT1.queryAndUpdate(
                ( BSONObject ) JSON.parse( "{'a': {'$gte': 100, '$lt': 200}}" ),
                null, null, ( BSONObject ) JSON.parse( "{'': 'a'}" ),
                ( BSONObject ) JSON.parse(
                        "{'$inc':{a: 1}, '$set': {'b': 'update r5s to r7s'}}" ),
                0, -1, 0, true );
        while ( cur.hasNext() ) {
            cur.getNext();
        }
        cur.close();

        List< BSONObject > T1ExpList = new ArrayList<>();
        T1ExpList.addAll( expDataList );
        TransUtils.updateList( T1ExpList, 3, "update r5s to r7s", 99, 100 );
        TransUtils.updateList( T1ExpList, 2, "update r5s to r7s", 100, 199 );

        TransUtils.queryAndCheck( clT1, "{'a': {$gte:0, $lt: 300}}",
                "{'_id': 1}", "{'': null}", T1ExpList );
        TransUtils.queryAndCheck( clT1, "{'a': {$gte:0, $lt: 300}}",
                "{'_id': 1}", "{'': 'a'}", T1ExpList );

        TransUtils.commitTransaction( T1 );
    }

    @AfterClass
    public void tearDown() {
        if ( T1 != null ) {
            T1.close();
        }
        if ( T2 != null ) {
            T2.close();
        }
        if ( T3 != null ) {
            T3.close();
        }
        if ( T4 != null ) {
            T4.close();
        }
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
