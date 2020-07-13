package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
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
 * @Description seqDB-20541 新表只读事务与只写事务并发
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20541 extends SdbTestBase {

    private String clName = "transCL_20541";
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private DBCollection clTR1 = null;
    private DBCollection clTW1 = null;

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        DBCollection cl = sdb.getCollectionSpace( csName )
                .createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @Test
    public void test() throws InterruptedException {
        TR1 = CommLib.getRandomSequoiadb();
        TW1 = CommLib.getRandomSequoiadb();

        clTR1 = TR1.getCollectionSpace( csName ).getCollection( clName );
        clTW1 = TW1.getCollectionSpace( csName ).getCollection( clName );

        TransUtils.beginTransaction( TR1 );
        TransUtils.beginTransaction( TW1 );

        List< BSONObject > expList = TransUtils.insertRandomDatas( clTW1, 0,
                100 );

        List< BSONObject > expList1 = new ArrayList<>();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 100}}",
                "{'_id': 1}", "{'': null}", expList1 );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 100}}",
                "{'_id': 1}", "{'': 'a'}", expList1 );

        TransUtils.commitTransaction( TW1 );

        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 100}}",
                "{'_id': 1}", "{'': null}", expList1 );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 100}}",
                "{'_id': 1}", "{'': 'a'}", expList1 );

        TransUtils.commitTransaction( TR1 );

        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 100}}",
                "{'_id': 1}", "{'': null}", expList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 100}}",
                "{'_id': 1}", "{'': 'a'}", expList );
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
