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
 * @Description seqDB-20464:RBS中存在非事务操作的老版本，老版本清理
 * @author zhaoyu
 * @date 2020.1.20
 */
@Test(groups = "rr")
public class Transaction20464 extends SdbTestBase {

    private String clName = "transCL_20464";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private int recordNum = 10000;

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        TransUtils.insertRandomLengthRecords( cl, recordNum, 10, 1024 );
        TransUtils.beginTransaction( sdb );
        TransUtils.insertRandomLengthRecords( cl, recordNum, recordNum * 2, 10,
                1024 );
        TransUtils.commitTransaction( sdb );
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

            // 一边更新一遍查询
            for ( int i = 0; i < TransUtils.loopNum; i++ ) {
                // 开启写事务
                TransUtils.beginTransaction( db1 );

                // 开启读事务
                TransUtils.beginTransaction( db2 );

                // 写事务更新记录
                String transID = TransUtils.getTransactionID( db1 );
                System.out.println( this.getClass().getName()
                        + " transID update:" + transID );
                cl1.update( null, "{$inc:{a:1}}", "{'':'a'}" );
                TransUtils.commitTransaction( db1 );
                cl1.update( null, "{$inc:{a:1}}", "{'':'a'}" );

                // 读事务读记录
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
