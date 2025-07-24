package com.sequoiadb.transaction.rr;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

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
public class Transaction20451A extends SdbTestBase {

    private String csName = "transCS_20451A";
    private String newCSName = "transCS_20451new";
    private String clName = "transCL_20451";
    private String newCLName = "transCL_20451new";
    private Sequoiadb sdb = null;
    private Sequoiadb TR1 = null;
    private Sequoiadb TW1 = null;
    private DBCollection cl = null;
    private DBCollection clTR1 = null;
    private DBCollection clTW1 = null;
    private int recordNum = 100;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = CommLib.getRandomSequoiadb();
        BSONObject options = new BasicBSONObject();
        options.put( "ShardingType", "hash" );
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "AutoSplit", true );
        cl = sdb.createCollectionSpace( csName ).createCollection( clName,
                options );
        expDataList = TransUtils.prepareDatas( sdb, cl, recordNum );
    }

    @Test
    public void test() throws InterruptedException {
        TR1 = CommLib.getRandomSequoiadb();
        TW1 = CommLib.getRandomSequoiadb();

        clTR1 = TR1.getCollectionSpace( csName ).getCollection( clName );
        clTW1 = TW1.getCollectionSpace( csName ).getCollection( clName );

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
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // rename cs
        sdb.renameCollectionSpace( csName, newCSName );
        clTR1 = TR1.getCollectionSpace( newCSName ).getCollection( clName );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // rename cl
        sdb.getCollectionSpace( newCSName ).renameCollection( clName,
                newCLName );
        clTR1 = TR1.getCollectionSpace( newCSName ).getCollection( newCLName );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // alter cl
        cl = sdb.getCollectionSpace( newCSName ).getCollection( newCLName );
        cl.alterCollection( new BasicBSONObject( "ReplSize", 7 ) );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // createIndex
        cl.createIndex( "b", "{b: 1}", false, false );

        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // dropIndex
        cl.dropIndex( "b" );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // createAutoIncrement
        cl.createAutoIncrement( ( BSONObject ) JSON
                .parse( "{Field: 'userID', Generated: 'always'}" ) );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clTR1, "{'a': {$gte: 0, $lt: 100}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // dropAutoIncrement
        cl.dropAutoIncrement( "userID" );
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
        sdb.dropCollectionSpace( newCSName );
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
