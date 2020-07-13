package com.sequoiadb.transaction.rrrbsserial;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-20457:第N号集合maxGlobTransID大于全局lowTransID，RBS写到第M个集合，M-N>2
 * @author zhaoyu
 * @date 2020.1.20
 */
@Test(groups = "rr")
public class Transaction20457 extends SdbTestBase {

    private String clName = "transCL_20457";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private int recordNum = 10000;
    private ArrayList< BSONObject > expDataList = new ArrayList<>();

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        TransUtils.insertRandomLengthRecords( cl, recordNum, 10, 1024 );
    }

    @Test
    public void test() {
        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        try {
            db1 = CommLib.getRandomSequoiadb();
            db2 = CommLib.getRandomSequoiadb();
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );

            // 更新记录在RBS中保存多个集合的老版本
            for ( int i = 0; i < TransUtils.loopNum; i++ ) {
                TransUtils.beginTransaction( db1 );
                cl.update( null, "{$inc:{a:1}}", null );
                TransUtils.commitTransaction( db1 );
            }

            // 作为后续事务查询的预期结果
            DBCursor cursor = cl.query( "", "", "{a:1}", "" );
            expDataList = TransUtils.getReadActList( cursor );

            // 开启读事务
            TransUtils.beginTransaction( db2 );
            String transID = TransUtils.getTransactionID( db2 );
            System.out.println(
                    this.getClass().getName() + " transID query:" + transID );

            // 由于集合加锁是写锁优化采取一边更新一边查询的方式(不使用读写并发线程)
            for ( int i = 0; i < TransUtils.loopNum; i++ ) {
                TransUtils.beginTransaction( db1 );
                cl1.update( null, "{$inc:{a:1}}", null );
                TransUtils.commitTransaction( db1 );
                TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}",
                        expDataList );
                System.out.println( this.getClass().getName() + " query exec: "
                        + i + " times." );
            }
            TransUtils.commitTransaction( db2 );
        } finally {
            db1.close();
            db2.close();
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
