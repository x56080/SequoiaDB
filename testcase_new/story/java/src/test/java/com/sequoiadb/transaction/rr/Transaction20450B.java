package com.sequoiadb.transaction.rr;

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
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-20450 select for update操作与事务隔离级别的验证
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20450B extends SdbTestBase {

    private String clName = "transCL_20450B";
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private Sequoiadb TW2 = null;
    private DBCollection cl = null;
    private DBCollection clTR1 = null;
    private DBCollection clTW1 = null;
    private DBCollection clTW2 = null;
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
        TW2 = CommLib.getRandomSequoiadb();

        clTR1 = TR1.getCollectionSpace( csName ).getCollection( clName );
        clTW1 = TW1.getCollectionSpace( csName ).getCollection( clName );
        clTW2 = TW2.getCollectionSpace( csName ).getCollection( clName );

        // 1 begin trans TR1 read
        TransUtils.beginTransaction( TR1 );

        // 2 begin trans TW1 upsert R1s to R3s
        TransUtils.beginTransaction( TW1 );
        clTW1.update( "{'a': {'$gte': 0, '$lt': 100}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r1s to r2s'}}",
                "{'': 'a'}" );
        TW1.rollback();

        // 3 TR1 query records
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        List< BSONObject > actList = new ArrayList<>();
        DBCursor cur = clTR1.query( null, null, new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "", "a" ), DBQuery.FLG_QUERY_FOR_UPDATE );

        while ( cur.hasNext() ) {
            actList.add( cur.getNext() );
        }
        cur.close();
        Assert.assertEquals( actList, expDataList );

        // 4 trans TR1 read 判断事务阻塞需先获取事务id
        TransUtils.beginTransaction( TW2 );
        String transactionID2 = TransUtils.getTransactionID( TW2 );
        UpdateThread updateThread = new UpdateThread();
        updateThread.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb, transactionID2 ) );

        TR1.rollback();

        Assert.assertTrue( updateThread.isSuccess() );
        TW2.rollback();

        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 102}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 102}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
    }

    @AfterClass
    public void tearDown() {
        if ( TR1 != null ) {
            TR1.close();
        }
        if ( TW1 != null ) {
            TW1.close();
        }
        if ( TW2 != null ) {
            TW2.close();
        }
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class UpdateThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            clTW2.update( "{'a': {'$gte': 0, '$lt': 100}}",
                    "{'$set': {'b': 'update r2s to r3s'}}", "{'': null}" );
        }
    }
}
