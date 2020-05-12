package com.sequoiadb.transaction.rrrbsserial;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-20462:全局lowTransID为写事务时，老版本清理
 * @author zhaoyu
 * @date 2020.1.20
 */
@Test(groups = "rr")
public class Transaction20462 extends SdbTestBase {

    private String clName = "transCL_20462";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private int recordNum = 10000;
    private ArrayList< BSONObject > expDataList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse( "{Compressed:false}" ) );
        cl.createIndex( "a", "{a:1}", false, false );
        TransUtils.insertRandomLengthRecords( cl, recordNum, 10, 1024 );
    }

    @Test
    public void test() {
        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        Sequoiadb db3 = null;

        try {
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl3 = db3.getCollectionSpace( csName )
                    .getCollection( clName );

            // 产生多个老版本
            for ( int i = 0; i < TransUtils.loopNum; i++ ) {
                TransUtils.beginTransaction( db1 );
                String transID = TransUtils.getTransactionID( db1 );
                System.out.println( "transID update:" + transID );
                cl1.update( "", "{$inc:{a:1}}", "{'':'a'}" );
                db1.commit();

            }

            // lowTrans为更新事务
            TransUtils.beginTransaction( db2 );
            BSONObject record1 = ( BSONObject ) JSON.parse(
                    "{_id:1000000,a:-1,b:1000000,name:'insert data in transaction but not commit'}" );
            cl2.insert( record1 );
            String transactionID = ( String ) db2
                    .getSnapshot( Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, "",
                            "{TransactionID:''}", "" )
                    .getCurrent().get( "TransactionID" );
            System.out.println( "lowTransID:" + transactionID );

            // 更新并执行查询
            for ( int i = 0; i < TransUtils.loopNum; i++ ) {
                TransUtils.beginTransaction( db3 );
                TransUtils.beginTransaction( db1 );
                String transID = TransUtils.getTransactionID( db1 );
                System.out.println( "transID update:" + transID );
                cl1.update( "{a:{$gt:0}}", "{$inc:{a:1}}", "{'':'a'}" );
                db1.commit();

                DBCursor cursor = cl3.query( "", "", "{a:1}", "{'':null}" );
                expDataList = TransUtils.getReadActList( cursor );
                TransUtils.queryAndCheck( cl3, "{a:1}", "{'':'a'}",
                        expDataList );
                db3.commit();
            }

        } finally {
            db1.commit();
            db2.commit();
            db3.commit();
            db1.close();
            db2.close();
            db3.close();
        }

    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
