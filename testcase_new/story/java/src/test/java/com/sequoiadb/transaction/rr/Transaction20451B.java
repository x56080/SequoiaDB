package com.sequoiadb.transaction.rr;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-20451 元数据操作，不影响事务读的隔离级别
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20451B extends SdbTestBase {

    private String csName = "transCS_20451B";
    private String mainCLName = "mainCL_20451";
    private String subCLName1 = "subCL_20451_1";
    private String subCLName2 = "subCL_20451_2";
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private CollectionSpace mainCS = null;
    private DBCollection mainCL = null;
    private DBCollection clTR1 = null;
    private DBCollection clTW1 = null;
    private int recordNum = 100;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        mainCS = sdb.createCollectionSpace( csName );
        mainCL = mainCS.createCollection( mainCLName, ( BSONObject ) JSON.parse(
                "{IsMainCL:true, ShardingType:'range', ShardingKey:{a:1}}" ) );
        mainCS.createCollection( subCLName1 );
        mainCS.createCollection( subCLName2 );
        mainCL.attachCollection( csName + "." + subCLName1, ( BSONObject ) JSON
                .parse( "{LowBound:{a: 0}, UpBound:{a: 200}}" ) );
        mainCL.createIndex( "a", "{a:-1}", false, false );
        expDataList = TransUtils.prepareDatas( sdb, mainCL, recordNum );
    }

    @Test
    public void test() throws InterruptedException {
        TR1 = CommLib.getRandomSequoiadb();
        TW1 = CommLib.getRandomSequoiadb();

        clTR1 = TR1.getCollectionSpace( csName ).getCollection( mainCLName );
        clTW1 = TW1.getCollectionSpace( csName ).getCollection( mainCLName );

        // 1 begin trans TR1
        TransUtils.beginTransaction( TR1 );

        // 2 begin trans TW1 upsert R1s to R2s
        TransUtils.beginTransaction( TW1 );
        clTW1.update( "{'a': {'$gte': 0, '$lt': 100}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r1s to r2s'}}",
                "{'': 'a'}" );
        TransUtils.commitTransaction( TW1 );

        // 3 trans TR1 read
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'a': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'a': 1}", "{'': 'a'}", expDataList );

        // attachCL
        mainCL.attachCollection( csName + "." + subCLName2, ( BSONObject ) JSON
                .parse( "{LowBound:{a: 200}, UpBound:{a: 400}}" ) );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'a': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'a': 1}", "{'': 'a'}", expDataList );

        // detachCL
        mainCL.detachCollection( csName + "." + subCLName2 );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'a': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
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
        sdb.dropCollectionSpace( csName );
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
