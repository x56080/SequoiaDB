package com.sequoiadb.transaction.rr;

import java.util.List;

import org.bson.BSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-20505 唯一索引已存在，老版本与当前版本唯一索引冲突
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20505 extends SdbTestBase {

    private String clName = "transCL_20505";
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
        cl.createIndex( "a", "{a:1}", true, false );
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
        clTW1.update( "{'a': {'$gte': 0, '$lt': 100}}", "{'$inc': {'a': 100}}}",
                "{'': 'a'}" );
        clTW1.update( "{'a': {'$gte': 100, '$lt': 200}}",
                "{'$inc': {'a': -100}}}", "{'': 'a'}" );
        TransUtils.commitTransaction( TW1 );

        // 3 begin trans TW2 upsert R1s to R3s
        TransUtils.beginTransaction( TW2 );
        clTW2.update( "{'a': {'$gte': 0, '$lt': 100}}", "{'$inc': {'a': 100}}}",
                "{'': 'a'}" );
        clTW2.delete( "{'a': {'$gte': 100, '$lt': 200}}", "{'': 'a'}" );
        List< BSONObject > datas = TransUtils.getPrepareDatas( 200 );
        clTW2.insert( datas );
        TransUtils.commitTransaction( TW2 );

        // 3 TR1 query records
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
        if ( TW2 != null ) {
            TW2.close();
        }
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

}
