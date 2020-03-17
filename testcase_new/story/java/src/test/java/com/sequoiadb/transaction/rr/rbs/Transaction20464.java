package com.sequoiadb.transaction.rr.rbs;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
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
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        TransUtils.insertRandomLengthRecords( cl, recordNum, 10, 1024 );
        sdb.beginTransaction();
        TransUtils.insertRandomLengthRecords( cl, recordNum, recordNum * 2, 10,
                1024 );
        sdb.commit();
    }

    // SEQUOIADBMAINSTREAM-5620
    @Test(enabled = false)
    public void test() {
        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        try {
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );

            // 一边更新一遍查询
            for ( int i = 0; i < TransUtils.loopNum; i++ ) {
                // 开启写事务
                db1.beginTransaction();

                // 开启读事务
                db2.beginTransaction();

                // 写事务更新记录
                String transID = TransUtils.getTransactionID( db1 );
                System.out.println( "transID update:" + transID );
                cl1.update( null, "{$inc:{a:1}}", "{'':'a'}" );
                db1.commit();
                cl1.update( null, "{$inc:{a:1}}", "{'':'a'}" );

                // 读事务读记录
                DBCursor cursor = cl2.query( "", "", "{a:1}", "{'':null}" );
                ArrayList< BSONObject > expDataList = TransUtils
                        .getReadActList( cursor );
                TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}",
                        expDataList );
                db2.commit();
            }
        } finally {
            db1.commit();
            db2.commit();
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
