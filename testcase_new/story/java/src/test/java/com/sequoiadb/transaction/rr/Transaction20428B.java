package com.sequoiadb.transaction.rr;

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
 * @Description seqDB-20428 只读事务与只写事务并发，只写事务覆盖：插入、更新、删除，且串行，不同时刻发起事务读，隔离级别为RR
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20428B extends SdbTestBase {

    private String clName = "transCL_20428B";
    private String mainCLName = "mainCL_20428B";
    private String subCLName1 = "subCL_20428B_1";
    private String subCLName2 = "subCL_20428B_2";
    private String hashCLName = "hashCL_20428B";
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
    private int recordNum = 3000;
    private List< BSONObject > expDataList = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                new Object[] { clName, "{a: 1}" },
                new Object[] { mainCLName, "{a: -1}" },
                new Object[] { hashCLName, "{a: 1}" } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
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
        TW1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        TW2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        TR1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        TR2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        TR3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        TR4 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        TR5 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        clTW1 = TW1.getCollectionSpace( csName ).getCollection( clName );
        clTW2 = TW2.getCollectionSpace( csName ).getCollection( clName );
        clTR1 = TR1.getCollectionSpace( csName ).getCollection( clName );
        clTR2 = TR2.getCollectionSpace( csName ).getCollection( clName );
        clTR3 = TR3.getCollectionSpace( csName ).getCollection( clName );
        clTR4 = TR4.getCollectionSpace( csName ).getCollection( clName );
        clTR5 = TR5.getCollectionSpace( csName ).getCollection( clName );

        // 1 trans TR1 read 0-3000
        TR1.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 2 begin trans TW1
        TW1.beginTransaction();

        // 3 trans TR2 read 0-3000
        TR2.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 4 trans TW1 insert R4S
        TransUtils.insertRandomDatas( clTW1, recordNum, recordNum + 1000 );
        clTW1.update( "{'a': {'$gte': 0, '$lt': 1000}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r1s to r5s'}}",
                "{'': 'a'}" );
        clTW1.delete( "{'a': {'$gte': 1000, '$lt': 2000}}", "{'': 'a'}" );
        TW1.rollback();

        // 5 trans TR3 read 2000-5000
        TR3.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 6 trans TW2 insert records
        TW2.beginTransaction();
        TransUtils.insertRandomDatas( clTW2, recordNum + 1000,
                recordNum + 2000 );
        clTW2.update( "{'a': {'$gte': 0, '$lt': 1000}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r5s to r7s'}}",
                "{'': 'a'}" );
        clTW2.delete( "{'a': {'$gte': 2000, '$lt': 3000}}", "{'': 'a'}" );

        // 7 trans TR4 read 2000-5000
        TR4.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR3, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR4, "{'a': {'$gte': 0, '$lt': 4000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 8 commit TR2
        TW2.rollback();

        // 9 trans TR5 read 3000-6000
        TR5.beginTransaction();
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 3000}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR2, "{'a': {'$gte': 0, '$lt': 3000}}",
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
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR5, "{'a': {'$gte': 0, '$lt': 5000}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        TR1.commit();
        TR2.commit();
        TR3.commit();
        TR4.commit();
        TR5.commit();
    }

    @AfterClass
    public void tearDown() {
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
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
