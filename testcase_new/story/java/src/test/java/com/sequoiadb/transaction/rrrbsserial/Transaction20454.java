package com.sequoiadb.transaction.rrrbsserial;

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
 * @Description seqDB-20454:RBS中写多个集合，第1个集合中的maxGlobTransID小于全局lowTransID，老版本清理
 * @author zhaoyu
 * @date 2020.1.20
 */
@Test(groups = "rr")
public class Transaction20454 extends SdbTestBase {

    private String clName = "transCL_20454";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private int recordNum = 10000;
    private List< BSONObject > expDataList = new ArrayList<>();

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        expDataList = TransUtils.insertRandomLengthRecords( cl, recordNum, 10,
                1024 );

    }

    @Test
    public void test() {
        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        try {
            // 开启读事务并获取事务ID
            db1 = CommLib.getRandomSequoiadb();
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            TransUtils.beginTransaction( db1 );
            String transID1 = TransUtils.getTransactionID( db1 );
            System.out.println(
                    this.getClass().getName() + " transID query:" + transID1 );

            // 创建写事务连接
            db2 = CommLib.getRandomSequoiadb();
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );

            // 集合的写锁优先于读锁,读写线程不采用线程的方式(会导致更新执行完成后才执行读事务)
            for ( int i = 0; i < TransUtils.loopNum; i++ ) {
                TransUtils.beginTransaction( db2 );
                String transID2 = TransUtils.getTransactionID( db2 );
                System.out.println( this.getClass().getName()
                        + " transID update:" + transID2 );
                cl2.update( null, "{$inc:{a:1}}", "{'':'a'}", 0 );
                TransUtils.commitTransaction( db2 );

                TransUtils.queryAndCheck( cl1, "{a:1}", "{'':null}",
                        expDataList );
                TransUtils.queryAndCheck( cl1, "{a:1}", "{'':'a'}",
                        expDataList );
            }
            TransUtils.commitTransaction( db1 );

        } finally {
            db1.close();
            db2.close();
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }
}
