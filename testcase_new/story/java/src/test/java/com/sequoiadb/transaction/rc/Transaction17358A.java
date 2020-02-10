package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-17358.java
 *              插入与更新并发，更新的记录同时匹配已提交记录及其他事务插入的记录，更新走索引，事务提交，过程中读 R3>R2>R1
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "rc")
public class Transaction17358A extends SdbTestBase {

    private String clName = "transCL_17358A";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private Sequoiadb sdb3 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private BSONObject insertR1 = new BasicBSONObject();
    private BSONObject insertR2 = new BasicBSONObject();
    private BSONObject updateR1 = new BasicBSONObject();
    private BSONObject updateR2 = new BasicBSONObject();
    private List< BSONObject > expDataList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17358A_1',a:1,b:1,c:1}" );
        insertR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17358A_2',a:2,b:2,c:2}" );
        updateR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17358A_1',a:3,b:3,c:1}" );
        updateR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17358A_2',a:4,b:4,c:2}" );
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {

        // 第一次非事务读正序查询的预期结果
        List< BSONObject > expPositiveReadList1 = new ArrayList< BSONObject >();
        expPositiveReadList1.add( insertR2 );
        expPositiveReadList1.add( updateR1 );

        // 第一次非事务读逆序查询的预期结果
        List< BSONObject > expReverseReadList1 = new ArrayList< BSONObject >();
        expReverseReadList1.add( updateR1 );
        expReverseReadList1.add( insertR2 );

        // 第二次非事务读正序查询的预期结果
        List< BSONObject > expPositiveReadList2 = new ArrayList< BSONObject >();
        expPositiveReadList2.add( updateR1 );
        expPositiveReadList2.add( updateR2 );

        // 第二次非事务读逆序查询的预期结果
        List< BSONObject > expReverseReadList2 = new ArrayList< BSONObject >();
        expReverseReadList2.add( updateR2 );
        expReverseReadList2.add( updateR1 );

        // 第一次非事务读正序查询的预期结果
        List< BSONObject > expPositiveReadList3 = new ArrayList< BSONObject >();
        expPositiveReadList3.add( insertR1 );
        expPositiveReadList3.add( insertR2 );

        // 第一次非事务读逆序查询的预期结果
        List< BSONObject > expReverseReadList3 = new ArrayList< BSONObject >();
        expReverseReadList3.add( insertR2 );
        expReverseReadList3.add( insertR1 );

        return new Object[][] {
                { "{'a': 1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': 1, b: 1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': 1, b: -1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': -1}", expPositiveReadList3, expReverseReadList3,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': -1, b: 1}", expPositiveReadList3, expReverseReadList3,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': -1, b: -1}", expPositiveReadList3, expReverseReadList3,
                        expPositiveReadList2, expReverseReadList2 },

        };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey, List< BSONObject > expPositiveReadList1,
            List< BSONObject > expReverseReadList1,
            List< BSONObject > expPositiveReadList2,
            List< BSONObject > expReverseReadList2 )
            throws InterruptedException {
        try {
            // 插入记录R1
            cl.insert( insertR1 );
            cl.createIndex( "a", indexKey, false, false );

            cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = sdb3.getCollectionSpace( csName ).getCollection( clName );

            // 开启事务
            sdb1.beginTransaction();
            sdb2.beginTransaction();
            sdb3.beginTransaction();

            // 事务1插入记录，R1>R2
            cl1.insert( insertR2 );

            // 事务2更新记录R1为R3，R2为R4,R4>R3>R2>R1
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    updateThread.getTransactionID() ) );

            // 事务1记录读，正序查询
            expDataList.clear();
            expDataList.add( insertR1 );
            expDataList.add( insertR2 );
            TransUtils.queryAndCheck( cl1, "{a: 1, b:-1}", "{'': null}",
                    expDataList );

            // 事务1索引读，正序查询
            TransUtils.queryAndCheck( cl1, "{a: 1, b:-1}", "{'': 'a'}",
                    expDataList );

            // 事务1记录读，逆序查询
            expDataList.clear();
            expDataList.add( insertR2 );
            expDataList.add( insertR1 );
            TransUtils.queryAndCheck( cl1, "{a: -1, b:1}", "{'': null}",
                    expDataList );

            // 事务1索引读，逆序查询
            TransUtils.queryAndCheck( cl1, "{a: -1, b:1}", "{'': 'a'}",
                    expDataList );

            // 事务3记录读，正序查询
            expDataList.clear();
            expDataList.add( insertR1 );
            TransUtils.queryAndCheck( cl3, "{a: -1, b:1}", "{'': null}",
                    expDataList );

            // 事务3索引读，正序查询
            TransUtils.queryAndCheck( cl3, "{a: -1, b:1}", "{'': 'a'}",
                    expDataList );

            // 事务3记录读，逆序查询
            TransUtils.queryAndCheck( cl3, "{a: -1, b:1}", "{'': null}",
                    expDataList );

            // 事务3索引读，逆序查询
            TransUtils.queryAndCheck( cl3, "{a: -1, b:1}", "{'': 'a'}",
                    expDataList );

            // 非事务记录读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1, b:-1}", "{'': null}",
                    expPositiveReadList1 );

            // 非事务索引读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1, b:-1}", "{'': 'a'}",
                    expPositiveReadList1 );

            // 非事务记录读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1, b:1}", "{'': null}",
                    expReverseReadList1 );

            // 非事务索引读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1, b:1}", "{'': 'a'}",
                    expReverseReadList1 );

            // 提交事务1
            sdb1.commit();
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );

            // 非事务记录读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1, b:-1}", "{'': null}",
                    expPositiveReadList2 );

            // 非事务索引读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1, b:-1}", "{'': 'a'}",
                    expPositiveReadList2 );

            // 非事务记录读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1, b:1}", "{'': null}",
                    expReverseReadList2 );

            // 非事务索引读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1, b:1}", "{'': 'a'}",
                    expReverseReadList2 );

            // 事务2记录读，正序查询
            expDataList.clear();
            expDataList.add( updateR1 );
            expDataList.add( updateR2 );
            TransUtils.queryAndCheck( cl2, "{a: 1, b:-1}", "{'': null}",
                    expDataList );

            // 事务2索引读，正序查询
            TransUtils.queryAndCheck( cl2, "{a: 1, b:-1}", "{'': 'a'}",
                    expDataList );

            // 事务2记录读，逆序查询
            expDataList.clear();
            expDataList.add( updateR2 );
            expDataList.add( updateR1 );
            TransUtils.queryAndCheck( cl2, "{a: -1, b:1}", "{'': null}",
                    expDataList );

            // 事务2索引读，逆序查询
            TransUtils.queryAndCheck( cl2, "{a: -1, b:1}", "{'': 'a'}",
                    expDataList );

            // 事务3记录读，正序查询
            expDataList.clear();
            expDataList.add( insertR1 );
            expDataList.add( insertR2 );
            TransUtils.queryAndCheck( cl3, "{a:1, b:-1}", "{'': null}",
                    expDataList );

            // 事务3索引读，正序查询
            TransUtils.queryAndCheck( cl3, "{a:1, b:-1}", "{'': 'a'}",
                    expDataList );

            // 事务3记录读，逆序查询
            expDataList.clear();
            expDataList.add( insertR2 );
            expDataList.add( insertR1 );
            TransUtils.queryAndCheck( cl3, "{a:-1, b:1}", "{'': null}",
                    expDataList );

            // 事务3索引读，逆序查询
            TransUtils.queryAndCheck( cl3, "{a:-1, b:1}", "{'': 'a'}",
                    expDataList );

            // 提交事务2
            sdb2.commit();

            // 非事务记录读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1, b:-1}", "{'': null}",
                    expPositiveReadList2 );

            // 非事务索引读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1, b:-1}", "{'': 'a'}",
                    expPositiveReadList2 );

            // 非事务记录读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1, b:1}", "{'': null}",
                    expReverseReadList2 );

            // 非事务索引读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1, b:1}", "{'': 'a'}",
                    expReverseReadList2 );

            // 事务3记录读，正序查询
            expDataList.clear();
            expDataList.add( updateR1 );
            expDataList.add( updateR2 );
            TransUtils.queryAndCheck( cl3, "{a: 1, b:-1}", "{'': null}",
                    expDataList );

            // 事务3索引读，正序查询
            TransUtils.queryAndCheck( cl3, "{a: 1, b:-1}", "{'': 'a'}",
                    expDataList );

            // 事务3记录读，逆序查询
            expDataList.clear();
            expDataList.add( updateR2 );
            expDataList.add( updateR1 );
            TransUtils.queryAndCheck( cl3, "{a: -1, b:1}", "{'': null}",
                    expDataList );

            // 事务3索引读，逆序查询
            TransUtils.queryAndCheck( cl3, "{a: -1, b:1}", "{'': 'a'}",
                    expDataList );

            // 提交事务3
            sdb3.commit();
        } finally {
            // 关闭事务连接
            sdb1.commit();
            sdb2.commit();
            sdb3.commit();

            // 删除索引
            if ( cl.isIndexExist( "a" ) ) {
                cl.dropIndex( "a" );
            }

            // 删除记录
            cl.truncate();

        }
    }

    @AfterClass
    public void tearDown() {
        if ( sdb1 != null ) {
            sdb1.close();
        }
        if ( sdb2 != null ) {
            sdb2.close();
        }
        if ( sdb3 != null ) {
            sdb3.close();
        }

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class UpdateThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            cl2.update( null, "{'$inc': {'a': 2, 'b': 2}}", "{'': 'a'}" );
        }
    }

}
