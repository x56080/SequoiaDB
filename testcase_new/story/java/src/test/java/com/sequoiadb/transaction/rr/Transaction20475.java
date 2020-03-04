package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-20475:匹配条件为新版本，隔离级别验证
 * @author zhaoyu
 * @date 2020.4.3
 */
@Test(groups = "rr")
public class Transaction20475 extends SdbTestBase {

    private String clName = "transCL_20475";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private int recordNum = 10;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        expDataList = TransUtils.prepareDatas( sdb, cl, recordNum );
    }

    @Test
    public void test() {

        Sequoiadb TW1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        Sequoiadb TR1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        DBCollection clTW1 = TW1.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection clTR1 = TR1.getCollectionSpace( csName )
                .getCollection( clName );

        try {

            // 1 trans TR1 read
            TR1.beginTransaction();
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': 0, '$lt': " + recordNum + "}}",
                    "{'_id': 1}", "{'': null}", expDataList );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': 0, '$lt': " + recordNum + "}}",
                    "{'_id': 1}", "{'': 'a'}", expDataList );

            // 2 begin trans TW1
            TW1.beginTransaction();
            clTW1.update( null, "{'$inc':{a: " + recordNum + "}}", null );
            TW1.commit();

            // 3 trans TR2 read
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': 0, '$lt':" + recordNum + "}}", "{'a': 1}",
                    "{'': null}", expDataList );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': 0, '$lt': " + recordNum + "}}", "{'a': 1}",
                    "{'': 'a'}", expDataList );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': " + recordNum + ", '$lt': " + recordNum * 2
                            + "}}",
                    "{'a': 1}", "{'': null}", new ArrayList< BSONObject >() );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': " + recordNum + ", '$lt': " + recordNum * 2
                            + "}}",
                    "{'a':1}", "{'': 'a'}", new ArrayList< BSONObject >() );

            // 2 begin trans TW1
            TW1.beginTransaction();
            clTW1.update( null, "{'$inc':{a: -" + recordNum + "}}", null );
            clTW1.update( null, "{'$inc':{a: 1}}", null );
            TW1.commit();

            // 3 trans TR2 read
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': 0, '$lt':" + recordNum + "}}", "{'a': 1}",
                    "{'': null}", expDataList );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': 0, '$lt': " + recordNum + "}}", "{'a': 1}",
                    "{'': 'a'}", expDataList );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': " + recordNum + ", '$lt': " + recordNum * 2
                            + "}}",
                    "{'a': 1}", "{'': null}", new ArrayList< BSONObject >() );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': " + recordNum + ", '$lt': " + recordNum * 2
                            + "}}",
                    "{'a':1}", "{'': 'a'}", new ArrayList< BSONObject >() );

            // 2 begin trans TW1
            TW1.beginTransaction();
            clTW1.update( null, "{'$inc':{a: -1}}", null );
            clTW1.update( null, "{'$inc':{a: -" + recordNum
                    + "}, '$set': {'b': 'update r1s to r4s'}}", null );
            TW1.commit();

            // 3 trans TR2 read
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': 0, '$lt':" + recordNum + "}}", "{'a': 1}",
                    "{'': null}", expDataList );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': 0, '$lt': " + recordNum + "}}", "{'a': 1}",
                    "{'': 'a'}", expDataList );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': " + recordNum + ", '$lt': " + recordNum * 2
                            + "}}",
                    "{'a': 1}", "{'': null}", new ArrayList< BSONObject >() );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': " + recordNum + ", '$lt': " + recordNum * 2
                            + "}}",
                    "{'a':1}", "{'': 'a'}", new ArrayList< BSONObject >() );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': -" + recordNum + ", '$lt': 0}}", "{'a': 1}",
                    "{'': null}", new ArrayList< BSONObject >() );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': -" + recordNum + ", '$lt': 0}}", "{'a':1}",
                    "{'': 'a'}", new ArrayList< BSONObject >() );

            // 2 begin trans TW1
            TW1.beginTransaction();
            clTW1.delete( "" );
            insertDatas( clTW1, recordNum );
            TW1.commit();

            // 3 trans TR2 read
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': 0, '$lt':" + recordNum + "}}", "{'a': 1}",
                    "{'': null}", expDataList );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': 0, '$lt': " + recordNum + "}}", "{'a': 1}",
                    "{'': 'a'}", expDataList );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': " + recordNum + ", '$lt': " + recordNum * 2
                            + "}}",
                    "{'a': 1}", "{'': null}", new ArrayList< BSONObject >() );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': " + recordNum + ", '$lt': " + recordNum * 2
                            + "}}",
                    "{'a':1}", "{'': 'a'}", new ArrayList< BSONObject >() );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': -" + recordNum + ", '$lt': 0}}", "{'a': 1}",
                    "{'': null}", new ArrayList< BSONObject >() );
            TransUtils.queryAndCheck( clTR1,
                    "{'a': {'$gte': -" + recordNum + ", '$lt': 0}}", "{'a':1}",
                    "{'': 'a'}", new ArrayList< BSONObject >() );
        } finally {
            TW1.commit();
            TR1.commit();
            TW1.close();
            TR1.close();
        }

    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private void insertDatas( DBCollection cl, int recordNums ) {
        ArrayList< BSONObject > expDatas = new ArrayList<>();
        for ( int i = -recordNums; i < recordNums; i++ ) {
            BSONObject data = ( BSONObject ) JSON.parse( "{_id:" + i + ", a:"
                    + i + ", b:'test trans rr mode" + i + "'}" );
            expDatas.add( data );
        }
        cl.insert( expDatas );
    }
}
