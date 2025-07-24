package com.sequoiadb.transaction.rrrbs;

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
 * @Description seqDB-20456:RBS中写多个集合，第2号集合切换到第3号集合，第1号集合maxGlobTransID大于全局lowTransID
 *              seqDB-21939:读事务开启时间点在写事务之后，但在写事务提交之前，老版本清理（该用例可通过该自动化用例随机覆盖测试用例）
 *              以下4个用例可修改变量loopNum以达到写翻转RBS的测试点，不提交CI，只在本地使用特殊版本进行测试
 *              seqDB-20458:RBS写到max个集合，第max-2个集合maxGlobTransID小于全局lowTransID，清理RBS
 *              seqDB-20459:RBS写翻转到第0个集合，第max-1个集合maxGlobTransID小于全局lowTransID
 *              seqDB-20460:RBS写翻转到第1个集合，第max个集合maxGlobTransID小于全局lowTransID
 *              seqDB-20461:RBS写翻转后，第0个集合maxGlobTransID大于等于全局lowTransID
 * @author zhaoyu
 * @date 2020.1.20
 */
@Test(groups = "rr")
public class Transaction20456 extends SdbTestBase {

    private String clName = "transCL_20456";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private int recordNum = 10000;

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
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            db2 = CommLib.getRandomSequoiadb();
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );

            // 集合的写锁优先于读锁,读写线程不采用线程的方式(会导致更新执行完成后才执行读事务)
            for ( int i = 0; i < TransUtils.loopNum; i++ ) {
                // 开启写事务
                TransUtils.beginTransaction( db1 );
                String transID1 = TransUtils.getTransactionID( db1 );
                System.out.println( this.getClass().getName()
                        + " transID write:" + transID1 );

                // 开启读事务
                TransUtils.beginTransaction( db2 );
                String transID2 = TransUtils.getTransactionID( db2 );
                System.out.println( this.getClass().getName()
                        + " transID query:" + transID2 );

                // 写事务更新记录集合中的索引，并提交;
                cl1.update( null, "{$inc:{a:1}}", null, 0 );
                TransUtils.commitTransaction( db1 );

                // 读事务读记录并比较结果
                DBCursor cursor = cl2.query( "", "", "{a:1}", "{'':null}" );
                ArrayList< BSONObject > expDataList = TransUtils
                        .getReadActList( cursor );
                TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}",
                        expDataList );
                TransUtils.commitTransaction( db2 );
            }
        } finally {
            TransUtils.commitTransaction( db1 );
            TransUtils.commitTransaction( db2 );
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
