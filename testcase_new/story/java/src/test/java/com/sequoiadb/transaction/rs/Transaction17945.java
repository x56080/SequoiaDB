package com.sequoiadb.transaction.rs;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17945:插入删除同一条记录的同时执行总账查询，删除走索引扫描，查询覆盖：表扫描、索引扫描
 * @date 2020-1-16
 * @author zhaoyu
 *
 */

@Test(groups = "rs")
public class Transaction17945 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17945";
    private String idxName = "idx17945";
    private DBCollection cl = null;
    private int insertNum = 100;
    private int loopNum = 1000;
    // 经过实际测试，由于写操作优先于读操作，设置并发数会导致读操作极少，测试点覆盖不到，并发数暂时设置为1
    private int threadNum = 1;
    private int expSum = 1000000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertData();
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'b':-1}" }, { "{'b':1}" } };

    }

    @Test(dataProvider = "index")
    public void test( String indexKey ) throws Exception {
        try {

            // 创建索引
            cl.createIndex( idxName, indexKey, false, false );

            // 开启 3 个并发事务
            ThreadExecutor threadExecutor = new ThreadExecutor( 3600000 );
            for ( int i = 0; i < threadNum; i++ ) {
                threadExecutor.addWorker( new InsertDeleteThread() );
                threadExecutor.addWorker( new QueryThread() );
            }
            threadExecutor.run();
        } finally {
            // 删除索引
            cl.dropIndex( idxName );
        }
    }

    private void insertData() {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < insertNum; i++ ) {
            BSONObject object = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:10000, b:" + i + "}" );
            records.add( object );
        }
        cl.insert( records );
    }

    private class InsertDeleteThread {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        @ExecuteOrder(step = 1, desc = "插入删除记录")
        private void insertDelete() {
            try {
                for ( int i = 0; i < loopNum * 2; i++ ) {
                    System.out.println( "testcase: "
                            + new Exception().getStackTrace()[ 0 ]
                                    .getClassName()
                            + " insert delete times:" + i );
                    int aId = ( int ) ( Math.random() * insertNum ) + insertNum;
                    int bId = ( int ) ( Math.random() * insertNum );
                    int cId = ( int ) ( Math.random() * insertNum ) - insertNum;

                    int aBalance = aId + 10000;
                    int bBalance = bId + 10000;
                    int cBalance = cId + 10000;

                    // 开启写事务
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    BSONObject object = ( BSONObject ) JSON.parse( "{_id:" + aId
                            + ", a:" + aBalance + ", b:" + aId + "}" );
                    cl.insert( object );
                    cl.delete( "{b:" + aId + "}", "{'':'" + idxName + "'}" );

                    object = ( BSONObject ) JSON
                            .parse( "{_id:" + ( bId + insertNum * 2 ) + ", a:"
                                    + bBalance + ", b:" + bId + "}" );
                    cl.insert( object );
                    cl.delete( "{_id:" + ( bId + insertNum * 2 ) + "}",
                            "{'':'$id'}" );

                    object = ( BSONObject ) JSON.parse( "{_id:" + cId + ", a:"
                            + cBalance + ", b:" + cId + "}" );
                    cl.insert( object );
                    cl.delete( "{b:" + cId + "}", "{'':'" + idxName + "'}" );

                    // 提交、回滚更新事务
                    if ( aId % 2 == 0 ) {
                        db.commit();
                    } else {
                        db.rollback();
                    }
                }
            } finally {
                db.commit();
                db.close();
                System.out.println( "testcase: "
                        + new Exception().getStackTrace()[ 0 ].getClassName()
                        + " insert delete thread end" + new Date() );
            }
        }
    }

    private class QueryThread {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        @ExecuteOrder(step = 1, desc = "查询记录总账")
        public void query() throws Exception {
            try {
                for ( int i = 0; i < loopNum; i++ ) {
                    System.out
                            .println( "testcase: "
                                    + new Exception().getStackTrace()[ 0 ]
                                            .getClassName()
                                    + " query times:" + i );
                    // 开启查询事务，表扫描
                    db.beginTransaction();
                    String sqlIdxScan = "select sum(a) as sum from " + csName
                            + "." + clName + " /*+use_index(NULL)*/";
                    DBCursor cursor = null;
                    List< BSONObject > actNums = null;
                    cursor = db.exec( sqlIdxScan );
                    actNums = TransUtils.getReadActList( cursor );
                    Assert.assertEquals( actNums.size(), 1 );
                    double sumValue = ( double ) actNums.get( 0 ).get( "sum" );
                    int sum = ( int ) sumValue;
                    db.commit();
                    if ( sum != expSum ) {
                        throw new Exception(
                                "TblScan check sum error, expect sum is "
                                        + expSum + ", but actual sum:" + sum );
                    }

                    // 开启查询事务，索引扫描
                    db.beginTransaction();
                    String sqlTblScan = "select sum(a) as sum from " + csName
                            + "." + clName + " /*+use_index(" + idxName + ")*/";
                    cursor = db.exec( sqlTblScan );
                    actNums = TransUtils.getReadActList( cursor );
                    Assert.assertEquals( actNums.size(), 1 );
                    sumValue = ( double ) actNums.get( 0 ).get( "sum" );
                    sum = ( int ) sumValue;
                    db.commit();
                    if ( sum != expSum ) {
                        throw new Exception(
                                "IdxScan check sum error, expect sum is "
                                        + expSum + ", but actual sum:" + +sum );
                    }

                }
            } finally {
                db.commit();
                db.closeAllCursors();
                db.close();
                System.out.println( "testcase: "
                        + new Exception().getStackTrace()[ 0 ].getClassName()
                        + " query thread end" + new Date() );
            }
        }
    }
}
