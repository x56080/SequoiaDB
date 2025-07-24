package com.sequoiadb.transaction.rr;

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
 * @Description seqDB-20448
 *              读事务中使用update、remove、findAndUpdate、findAndRemove，未匹配到记录更新及删除，
 *              隔离级别验证
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20448A extends SdbTestBase {

    private String clName = "transCL_20448A";
    private Sequoiadb sdb = null;
    private Sequoiadb T1 = null;
    private Sequoiadb T2 = null;
    private DBCollection cl = null;
    private DBCollection clT1 = null;
    private DBCollection clT2 = null;
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
        T1 = CommLib.getRandomSequoiadb();
        T2 = CommLib.getRandomSequoiadb();

        clT1 = T1.getCollectionSpace( csName ).getCollection( clName );
        clT2 = T2.getCollectionSpace( csName ).getCollection( clName );

        // 1 begin trans T1 read
        TransUtils.beginTransaction( T1 );
        TransUtils.queryAndCheck( clT1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clT1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 2 begin trans T2 update R1S to R2s
        TransUtils.beginTransaction( T2 );
        clT2.update( "{'a': {'$gte': 0, '$lt': 100}}", "{'$inc': {'a': 100}}",
                "{'': 'a'}" );
        TransUtils.commitTransaction( T2 );

        // T1 read
        TransUtils.queryAndCheck( clT1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clT1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        clT1.update( "{'a': {'$gte': 0, '$lt': 100}}", "{'$inc': {'a': 200}}",
                "{'': 'a'}" );

        DBCursor cur = clT1.queryAndUpdate(
                ( BSONObject ) JSON.parse( "{'a': {'$gte': 0, '$lt': 100}}" ),
                null, null, ( BSONObject ) JSON.parse( "{'': 'a'}" ),
                ( BSONObject ) JSON
                        .parse( "{'$set': {'b': 'update r1s to r3s'}}" ),
                0, -1, 0, true );
        while ( cur.hasNext() ) {
            cur.getNext();
        }
        cur.close();

        clT1.delete( "{'a': {'$gte': 0, '$lt': 100}}", "{'': 'a'}" );

        DBCursor cur1 = clT1.queryAndRemove(
                ( BSONObject ) JSON.parse( "{'a': {'$gte': 0, '$lt': 100}}" ),
                null, null, ( BSONObject ) JSON.parse( "{'': 'a'}" ), 0, -1,
                0 );
        while ( cur1.hasNext() ) {
            cur.getNext();
        }
        cur1.close();

        TransUtils.queryAndCheck( clT1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clT1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

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
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
