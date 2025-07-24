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
 * @Description seqDB-20506 唯一索引已存在，老版本与当前版本唯一索引冲突
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20506 extends SdbTestBase {

    private String clName = "transCL_20506";
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private Sequoiadb TW2 = null;
    private DBCollection cl = null;
    private DBCollection clTR1 = null;
    private DBCollection clTW1 = null;
    private DBCollection clTW2 = null;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        TransUtils.beginTransaction( sdb );
        expDataList = TransUtils.insertRandomDatas( cl, 0, 300 );
        TransUtils.commitTransaction( sdb );
        expDataList.addAll( TransUtils.insertRandomDatas( cl, 300, 700 ) );
        TransUtils.beginTransaction( sdb );
        expDataList.addAll( TransUtils.insertRandomDatas( cl, 700, 1000 ) );
        TransUtils.commitTransaction( sdb );
    }

    @Test
    public void test() throws InterruptedException {
        TR1 = CommLib.getRandomSequoiadb();
        TW1 = CommLib.getRandomSequoiadb();
        TW2 = CommLib.getRandomSequoiadb();

        clTR1 = TR1.getCollectionSpace( csName ).getCollection( clName );
        clTW1 = TW1.getCollectionSpace( csName ).getCollection( clName );
        clTW2 = TW2.getCollectionSpace( csName ).getCollection( clName );

        // 2 begin trans TR1 read
        TransUtils.beginTransaction( TR1 );

        // 3 begin trans TW1 upsert R1s to R3s
        TransUtils.beginTransaction( TW1 );
        clTW1.update( null, "{'$inc': {'a': 1}}}", "{'': 'a'}" );
        TransUtils.commitTransaction( TW1 );

        // 4 create unique index
        cl.createIndex( "a", "{a: 1}", true, false );

        // 5 TR1 read
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 1000}}",
                "{'a': 1}", "{'': null}", expDataList );
        TransUtils.checkQueryResultOnly( clTR1, "{'a': {$gte: 0, $lt: 1000}}",
                "{'a': 1}", "{'': 'a'}", expDataList );

        // 6 begin trans TW2 upsert R2s to R3s
        TransUtils.beginTransaction( TW2 );
        clTW2.update( null, "{'$inc': {'b': 1}}}", "{'': 'a'}" );
        TransUtils.commitTransaction( TW2 );

        // 7 create unique index
        cl.createIndex( "b", "{b: 1}", true, false );

        // 3 TR1 query records
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 1000}}",
                "{'a': 1}", "{'': null}", expDataList );
        TransUtils.checkQueryResultOnly( clTR1, "{'a': {$gte: 0, $lt: 1000}}",
                "{'a': 1}", "{'': 'a'}", expDataList );

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
