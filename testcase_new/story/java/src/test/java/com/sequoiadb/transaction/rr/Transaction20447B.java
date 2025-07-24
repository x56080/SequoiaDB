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
 * @Description seqDB-20447 读事务中使用remove，隔离级别验证
 * @author luweikang
 * @date 2020.1.15
 */
@Test(groups = "rr")
public class Transaction20447B extends SdbTestBase {

    private String clName = "transCL_20447B";
    private Sequoiadb sdb = null;
    private Sequoiadb T1 = null;
    private Sequoiadb T2 = null;
    private Sequoiadb T3 = null;
    private Sequoiadb T4 = null;
    private DBCollection cl = null;
    private DBCollection clT1 = null;
    private DBCollection clT2 = null;
    private DBCollection clT3 = null;
    private DBCollection clT4 = null;
    private int recordNum = 300;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        expDataList = TransUtils.prepareDatas( sdb, cl, recordNum );
    }

    @Test
    public void test() {
        T1 = CommLib.getRandomSequoiadb();
        T2 = CommLib.getRandomSequoiadb();
        T3 = CommLib.getRandomSequoiadb();
        T4 = CommLib.getRandomSequoiadb();

        clT1 = T1.getCollectionSpace( csName ).getCollection( clName );
        clT2 = T2.getCollectionSpace( csName ).getCollection( clName );
        clT3 = T3.getCollectionSpace( csName ).getCollection( clName );
        clT4 = T4.getCollectionSpace( csName ).getCollection( clName );

        // 1 begin trans T1 read
        TransUtils.beginTransaction( T1 );
        TransUtils.queryAndCheck( clT1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clT1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        // 2 begin trans T2 remove R1s
        TransUtils.beginTransaction( T2 );
        clT2.delete( "{'a': {'$gte': 0, '$lt': 100}}", "{'': 'a'}" );
        T2.rollback();

        // 3 begin trans T3 update R2s to R4s
        TransUtils.beginTransaction( T3 );
        clT3.update( "{'a': {'$gte': 100, '$lt': 200}}",
                "{'$inc':{a: 1}, '$set': {'b': 'update r2s to r4s'}}",
                "{'': 'a'}" );
        T3.rollback();

        // 4 begin trans T4 update R3s to R6s
        TransUtils.beginTransaction( T4 );
        clT4.delete( "{'a': {'$gte': 200, '$lt': 300}}", "{'': 'a'}" );
        T4.rollback();

        // T1 read the records
        TransUtils.queryAndCheck( clT1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", expDataList );
        TransUtils.queryAndCheck( clT1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", expDataList );

        clT1.delete( "{'a': {'$gte': 100, '$lt': 200}}", "{'': 'a'}" );
        List< BSONObject > T1ExpList = new ArrayList<>();
        T1ExpList.addAll( expDataList );
        TransUtils.removeList( T1ExpList, 100, 200 );

        TransUtils.queryAndCheck( clT1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': null}", T1ExpList );
        TransUtils.queryAndCheck( clT1, "{'a': {'$gte': 0, '$lt': 300}}",
                "{'_id': 1}", "{'': 'a'}", T1ExpList );

        TransUtils.commitTransaction( T1 );
    }

    @AfterClass
    public void tearDown() {
        if ( T1 != null ) {
            T1.close();
        }
        if ( T2 != null ) {
            T2.close();
        }
        if ( T3 != null ) {
            T3.close();
        }
        if ( T4 != null ) {
            T4.close();
        }
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
