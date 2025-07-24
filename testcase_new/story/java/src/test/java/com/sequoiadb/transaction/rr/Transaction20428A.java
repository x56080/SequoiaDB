package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-20428 只读事务与只写事务并发，只写事务覆盖：插入、更新、删除，且串行，不同时刻发起事务读，隔离级别为RR
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20428A extends SdbTestBase {

    private String hashCLName = "hashCL_20428A";
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private Sequoiadb TR2 = null;
    private Sequoiadb TR3 = null;
    private Sequoiadb TW2 = null;
    private Sequoiadb TR4 = null;
    private Sequoiadb TR5 = null;
    private DBCollection cl = null;
    private DBCollection clTW1 = null;
    private DBCollection clTW2 = null;
    private DBCollection clTR1 = null;
    private DBCollection clTR2 = null;
    private DBCollection clTR3 = null;
    private DBCollection clTR4 = null;
    private DBCollection clTR5 = null;
    private int recordNum = 300;
    private List< BSONObject > expDataList = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] { new Object[] { hashCLName, "{a: -1}" } };
    }

    @BeforeClass
    public void setUp() {
        sdb = CommLib.getRandomSequoiadb();
        if ( !CommLib.isStandAlone( sdb ) ) {
            TransUtils.createHashCL( sdb, csName, hashCLName );
        }
    }

    @Test(dataProvider = "clNameProvider")
    public void test( String clName, String indexKey )
            throws InterruptedException {
        if ( CommLib.isStandAlone( sdb ) ) {
            if ( clName.equals( hashCLName ) )
                throw new SkipException( "is standalone skip testcase!" );
        }

        cl = sdb.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", indexKey, false, false );
        expDataList = TransUtils.prepareDatas( sdb, cl, recordNum );

        TW1 = CommLib.getRandomSequoiadb();
        TW2 = CommLib.getRandomSequoiadb();
        TR1 = CommLib.getRandomSequoiadb();
        TR2 = CommLib.getRandomSequoiadb();
        TR3 = CommLib.getRandomSequoiadb();
        TR4 = CommLib.getRandomSequoiadb();
        TR5 = CommLib.getRandomSequoiadb();

        clTW1 = TW1.getCollectionSpace( csName ).getCollection( clName );
        clTW2 = TW2.getCollectionSpace( csName ).getCollection( clName );
        clTR1 = TR1.getCollectionSpace( csName ).getCollection( clName );
        clTR2 = TR2.getCollectionSpace( csName ).getCollection( clName );
        clTR3 = TR3.getCollectionSpace( csName ).getCollection( clName );
        clTR4 = TR4.getCollectionSpace( csName ).getCollection( clName );
        clTR5 = TR5.getCollectionSpace( csName ).getCollection( clName );

        // 1 trans TR1 read 0-300
        TransUtils.beginTransaction( TR1 );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 2 begin trans TW1
        TransUtils.beginTransaction( TW1 );

        // 3 trans TR2 read 0-300
        TransUtils.beginTransaction( TR2 );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 4 trans TW1 insert R4S
        List< BSONObject > tw1InsertList = TransUtils.insertRandomDatas( clTW1,
                recordNum, recordNum + 100 );
        clTW1.update( "{'a': {'$gte': 0, '$lt': 100}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r1s to r5s'}}",
                "{'': 'a'}" );
        clTW1.delete( "{'a': {'$gte': 100, '$lt': 200}}", "{'': 'a'}" );
        TransUtils.commitTransaction( TW1 );
        List< BSONObject > tw1ExpList = new ArrayList<>();
        tw1ExpList.addAll( expDataList );
        tw1ExpList.addAll( tw1InsertList );
        TransUtils.updateList( tw1ExpList, 1, "update r1s to r5s", 0, 100 );
        TransUtils.removeList( tw1ExpList, 99, 200 );

        // 5 trans TR3 read 200-500
        TransUtils.beginTransaction( TR3 );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 400}}",
                "{'_id': 1}", "{'': null}", tw1ExpList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 400}}",
                "{'_id': 1}", "{'': 'a'}", tw1ExpList );

        // 6 trans TW2 insert records
        TransUtils.beginTransaction( TW2 );
        List< BSONObject > tw2InsertList = TransUtils.insertRandomDatas( clTW2,
                recordNum + 100, recordNum + 200 );
        clTW2.update( "{'a': {'$gte': 0, '$lt': 100}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r5s to r7s'}}",
                "{'': 'a'}" );
        clTW2.delete( "{'a': {'$gte': 200, '$lt': 300}}", "{'': 'a'}" );

        // 7 trans TR4 read 200-500
        TransUtils.beginTransaction( TR4 );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 400}}",
                "{'_id': 1}", "{'': null}", tw1ExpList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 400}}",
                "{'_id': 1}", "{'': 'a'}", tw1ExpList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 400}}",
                "{'_id': 1}", "{'': null}", tw1ExpList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 400}}",
                "{'_id': 1}", "{'': 'a'}", tw1ExpList );

        // 8 commit TR2
        TransUtils.commitTransaction( TW2 );
        List< BSONObject > tw2ExpList = new ArrayList<>();
        tw2ExpList.addAll( tw1ExpList );
        tw2ExpList.addAll( tw2InsertList );
        TransUtils.updateList( tw2ExpList, 1, "update r5s to r7s", 0, 100 );
        TransUtils.removeList( tw2ExpList, 99, 199 );

        // 9 trans TR5 read 300-600
        TransUtils.beginTransaction( TR5 );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 400}}",
                "{'_id': 1}", "{'': null}", tw1ExpList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 400}}",
                "{'_id': 1}", "{'': 'a'}", tw1ExpList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 400}}",
                "{'_id': 1}", "{'': null}", tw1ExpList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 400}}",
                "{'_id': 1}", "{'': 'a'}", tw1ExpList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 500}}",
                "{'_id': 1}", "{'': null}", tw2ExpList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 500}}",
                "{'_id': 1}", "{'': 'a'}", tw2ExpList );

        TransUtils.commitTransaction( TR1 );
        TransUtils.commitTransaction( TR2 );
        TransUtils.commitTransaction( TR3 );
        TransUtils.commitTransaction( TR4 );
        TransUtils.commitTransaction( TR5 );
    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( hashCLName );
        if ( TR1 != null ) {
            TR1.close();
        }
        if ( TW1 != null ) {
            TW1.close();
        }
        if ( TR2 != null ) {
            TR2.close();
        }
        if ( TR3 != null ) {
            TR3.close();
        }
        if ( TW2 != null ) {
            TW2.close();
        }
        if ( TR4 != null ) {
            TR4.close();
        }
        if ( TR5 != null ) {
            TR5.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
