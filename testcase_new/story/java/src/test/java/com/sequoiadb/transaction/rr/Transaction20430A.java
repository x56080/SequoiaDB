package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-20430
 *              只读事务与只写事务并发，只写事务覆盖：插入、更新、删除，TW2事务与TW3事务存在交集，不同时刻发起事务读，隔离级别为RR
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20430A extends SdbTestBase {

    private String clName = "transCL_20430A";
    private String mainCLName = "mainCL_20430A";
    private String subCLName1 = "subCL_20430A_1";
    private String subCLName2 = "subCL_20430A_2";
    private String hashCLName = "hashCL_20430A";
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private Sequoiadb TW2 = null;
    private Sequoiadb TW3 = null;
    private Sequoiadb TR2 = null;
    private Sequoiadb TR3 = null;
    private Sequoiadb TR4 = null;
    private Sequoiadb TR5 = null;
    private Sequoiadb TR6 = null;
    private Sequoiadb TR7 = null;
    private Sequoiadb TR8 = null;
    private Sequoiadb TR9 = null;
    private DBCollection cl = null;
    private DBCollection clTW1 = null;
    private DBCollection clTW2 = null;
    private DBCollection clTW3 = null;
    private DBCollection clTR1 = null;
    private DBCollection clTR2 = null;
    private DBCollection clTR3 = null;
    private DBCollection clTR4 = null;
    private DBCollection clTR5 = null;
    private DBCollection clTR6 = null;
    private DBCollection clTR7 = null;
    private DBCollection clTR8 = null;
    private DBCollection clTR9 = null;
    private int recordNum = 4000;
    private List< BSONObject > expDataList = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                new Object[] { clName, "{a: -1}" },
                new Object[] { mainCLName, "{a: 1}" },
                new Object[] { hashCLName, "{a: -1}" } };
    }

    @BeforeClass
    public void setUp() {
        sdb = CommLib.getRandomSequoiadb();
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        DBCollection cl = cs.createCollection( clName );
        if ( !CommLib.isStandAlone( sdb ) ) {
            DBCollection hashCL = TransUtils.createHashCL( sdb, csName,
                    hashCLName );
            DBCollection mainCL = TransUtils.createMainCL( sdb, csName,
                    mainCLName, subCLName1, subCLName2, 500 );
            TransUtils.prepareDatas( sdb, hashCL, recordNum );
            TransUtils.prepareDatas( sdb, mainCL, recordNum );
        }
        expDataList = TransUtils.prepareDatas( sdb, cl, recordNum );
    }

    @Test(dataProvider = "clNameProvider")
    public void test( String clName, String indexKey ) {
        if ( CommLib.isStandAlone( sdb ) ) {
            if ( clName.equals( mainCLName ) || clName.equals( hashCLName ) )
                throw new SkipException( "is standalone skip testcase!" );
        }

        cl = sdb.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", indexKey, false, false );
        TW1 = CommLib.getRandomSequoiadb();
        TW2 = CommLib.getRandomSequoiadb();
        TW3 = CommLib.getRandomSequoiadb();
        TR1 = CommLib.getRandomSequoiadb();
        TR2 = CommLib.getRandomSequoiadb();
        TR3 = CommLib.getRandomSequoiadb();
        TR4 = CommLib.getRandomSequoiadb();
        TR5 = CommLib.getRandomSequoiadb();
        TR6 = CommLib.getRandomSequoiadb();
        TR7 = CommLib.getRandomSequoiadb();
        TR8 = CommLib.getRandomSequoiadb();
        TR9 = CommLib.getRandomSequoiadb();

        clTW1 = TW1.getCollectionSpace( csName ).getCollection( clName );
        clTW2 = TW2.getCollectionSpace( csName ).getCollection( clName );
        clTW3 = TW3.getCollectionSpace( csName ).getCollection( clName );
        clTR1 = TR1.getCollectionSpace( csName ).getCollection( clName );
        clTR2 = TR2.getCollectionSpace( csName ).getCollection( clName );
        clTR3 = TR3.getCollectionSpace( csName ).getCollection( clName );
        clTR4 = TR4.getCollectionSpace( csName ).getCollection( clName );
        clTR5 = TR5.getCollectionSpace( csName ).getCollection( clName );
        clTR6 = TR6.getCollectionSpace( csName ).getCollection( clName );
        clTR7 = TR7.getCollectionSpace( csName ).getCollection( clName );
        clTR8 = TR8.getCollectionSpace( csName ).getCollection( clName );
        clTR9 = TR9.getCollectionSpace( csName ).getCollection( clName );

        // 1 trans TR1 read
        TR1.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 2 begin trans TW1
        TW1.beginTransaction();

        // 3 trans TR2 read
        TR2.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 4 begin trans TW2, insert R5s
        TW2.beginTransaction();
        List< BSONObject > tw2InsertList = TransUtils.insertRandomDatas( clTW2,
                recordNum, recordNum + 1000 );
        clTW2.update( "{'a': {'$gte': 0, '$lt': 1000}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r1s to r6s'}}",
                "{'': 'a'}" );
        clTW2.delete( "{'a': {'$gte': 1000, '$lt': 2000}}", "{'': 'a'}" );

        // 5 trans TR3 read
        TR3.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 6 begin trans TW3
        TW3.beginTransaction();

        // 7 trans TR4 read
        TR4.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 8 commit TW2, begin trans TR5 read
        TW2.commit();
        List< BSONObject > tw2ExpList = new ArrayList< >();
        tw2ExpList.addAll( expDataList );
        TransUtils.updateList( tw2ExpList, 1, "update r1s to r6s", 0, 1000 );
        TransUtils.removeList( tw2ExpList, 999, 2000 );
        tw2ExpList.addAll( tw2InsertList );

        TR5.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': null}", tw2ExpList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': 'a'}", tw2ExpList );

        // 9 TW3 insert R7s, begin TR6 read
        List< BSONObject > tw3TnsertList = TransUtils.insertRandomDatas( clTW3,
                recordNum + 1000, recordNum + 2000 );
        clTW3.update( "{'a': {'$gte': 0, '$lt': 1000}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r6s to r8s'}}",
                "{'': 'a'}" );
        clTW3.delete( "{'a': {'$gte': 2000, '$lt': 3000}}", "{'': 'a'}" );

        TR6.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': null}", tw2ExpList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': 'a'}", tw2ExpList );
        TransUtils.queryAndCheck( clTR6, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': null}", tw2ExpList );
        TransUtils.queryAndCheck( clTR6, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': 'a'}", tw2ExpList );

        // 10 commit TW3, begin TR7 read
        TW3.commit();
        List< BSONObject > tw3ExpList = new ArrayList< >();
        tw3ExpList.addAll( tw2ExpList );
        TransUtils.updateList( tw3ExpList, 1, "update r6s to r8s", 0, 999 );
        TransUtils.removeList( tw3ExpList, 999, 1999 );
        tw3ExpList.addAll( tw3TnsertList );

        TR7.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': null}", tw2ExpList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': 'a'}", tw2ExpList );
        TransUtils.queryAndCheck( clTR6, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': null}", tw2ExpList );
        TransUtils.queryAndCheck( clTR6, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': 'a'}", tw2ExpList );
        TransUtils.queryAndCheck( clTR7, "{'a': {'$gte': 0, '$lt': 6000}}",
                "{'_id': 1}", "{'': null}", tw3ExpList );
        TransUtils.queryAndCheck( clTR7, "{'a': {'$gte': 0, '$lt': 6000}}",
                "{'_id': 1}", "{'': 'a'}", tw3ExpList );

        // 11 TW1 insert TR9s, begin trans TR8
        List< BSONObject > tw1TnsertList = TransUtils.insertRandomDatas( clTW1,
                recordNum + 2000, recordNum + 3000 );
        clTW1.update( "{'a': {'$gte': 0, '$lt': 1000}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r8s to r10s'}}",
                "{'': 'a'}" );
        clTW1.delete( "{'a': {'$gte': 3000, '$lt': 4000}}", "{'': 'a'}" );

        TR8.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': null}", tw2ExpList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': 'a'}", tw2ExpList );
        TransUtils.queryAndCheck( clTR6, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': null}", tw2ExpList );
        TransUtils.queryAndCheck( clTR6, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': 'a'}", tw2ExpList );
        TransUtils.queryAndCheck( clTR7, "{'a': {'$gte': 0, '$lt': 6000}}",
                "{'_id': 1}", "{'': null}", tw3ExpList );
        TransUtils.queryAndCheck( clTR7, "{'a': {'$gte': 0, '$lt': 6000}}",
                "{'_id': 1}", "{'': 'a'}", tw3ExpList );
        TransUtils.queryAndCheck( clTR8, "{'a': {'$gte': 0, '$lt': 6000}}",
                "{'_id': 1}", "{'': null}", tw3ExpList );
        TransUtils.queryAndCheck( clTR8, "{'a': {'$gte': 0, '$lt': 6000}}",
                "{'_id': 1}", "{'': 'a'}", tw3ExpList );

        // 12 commit TW1, begin trans TR9 read
        TW1.commit();
        List< BSONObject > tw1ExpList = new ArrayList< >();
        tw1ExpList.addAll( tw3ExpList );
        TransUtils.updateList( tw1ExpList, 1, "update r8s to r10s", 0, 998 );
        TransUtils.removeList( tw1ExpList, 999, 1999 );
        tw1ExpList.addAll( tw1TnsertList );

        TR9.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': null}", tw2ExpList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': 'a'}", tw2ExpList );
        TransUtils.queryAndCheck( clTR6, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': null}", tw2ExpList );
        TransUtils.queryAndCheck( clTR6, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': 'a'}", tw2ExpList );
        TransUtils.queryAndCheck( clTR7, "{'a': {'$gte': 0, '$lt': 6000}}",
                "{'_id': 1}", "{'': null}", tw3ExpList );
        TransUtils.queryAndCheck( clTR7, "{'a': {'$gte': 0, '$lt': 6000}}",
                "{'_id': 1}", "{'': 'a'}", tw3ExpList );
        TransUtils.queryAndCheck( clTR8, "{'a': {'$gte': 0, '$lt': 6000}}",
                "{'_id': 1}", "{'': null}", tw3ExpList );
        TransUtils.queryAndCheck( clTR8, "{'a': {'$gte': 0, '$lt': 6000}}",
                "{'_id': 1}", "{'': 'a'}", tw3ExpList );
        TransUtils.queryAndCheck( clTR9, "{'a': {'$gte': 0, '$lt': 7000}}",
                "{'_id': 1}", "{'': null}", tw1ExpList );
        TransUtils.queryAndCheck( clTR9, "{'a': {'$gte': 0, '$lt': 7000}}",
                "{'_id': 1}", "{'': 'a'}", tw1ExpList );

        TR1.commit();
        TR2.commit();
        TR3.commit();
        TR4.commit();
        TR5.commit();
        TR6.commit();
        TR7.commit();
        TR8.commit();
        TR9.commit();
    }

    @AfterClass
    public void tearDown() {
        if ( TW1 != null ) {
            TW1.close();
        }
        if ( TW2 != null ) {
            TW2.close();
        }
        if ( TW3 != null ) {
            TW3.close();
        }
        if ( TR1 != null ) {
            TR1.close();
        }
        if ( TR2 != null ) {
            TR2.close();
        }
        if ( TR3 != null ) {
            TR3.close();
        }
        if ( TR4 != null ) {
            TR4.close();
        }
        if ( TR5 != null ) {
            TR5.close();
        }
        if ( TR6 != null ) {
            TR6.close();
        }
        if ( TR7 != null ) {
            TR7.close();
        }
        if ( TR8 != null ) {
            TR8.close();
        }
        if ( TR9 != null ) {
            TR9.close();
        }
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
