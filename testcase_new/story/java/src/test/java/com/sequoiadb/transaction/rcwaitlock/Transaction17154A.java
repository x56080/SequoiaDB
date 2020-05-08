package com.sequoiadb.transaction.rcwaitlock;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-17154: 更新记录与读记录并发，事务提交
 * @author xiaoni Zhao
 * @date 2019-1-22
 */

@Test(groups = "rcwaitlock")
public class Transaction17154A extends SdbTestBase {
    private String clName = "cl_17154A";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private Sequoiadb db4 = null;
    private Sequoiadb db5 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl4 = null;
    private DBCollection cl5 = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db4 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db5 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl3 = db3.getCollectionSpace( csName ).getCollection( clName );
        cl4 = db4.getCollectionSpace( csName ).getCollection( clName );
        cl5 = db5.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        cl.insert( "{_id:1, a:1, b:1}" );
    }

    @Test
    public void test() throws InterruptedException {
        // 开启事务1及事务2
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );
        TransUtils.beginTransaction( db3 );
        TransUtils.beginTransaction( db4 );
        TransUtils.beginTransaction( db5 );

        // 事务1更新索引字段的值
        cl1.update( null, "{$set:{a:2}}", "{'':'a'}" );
        BSONObject updateR1 = ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" );
        expList.add( updateR1 );

        // 事务匹配条件使用更新后值
        Query read1 = new Query( cl2, "{a:2}", "{'':null}", expList );
        read1.start();
        Query read2 = new Query( cl3, "{a:2}", "{'':'a'}", expList );
        read2.start();

        // 事务匹配条件使用更新更新前值
        Query read3 = new Query( cl4, "{a:1}", "{'':null}",
                new ArrayList< BSONObject >() );
        read3.start();
        Query read4 = new Query( cl5, "{a:1}", "{'':'a'}",
                new ArrayList< BSONObject >() );
        read4.start();

        // 查询均等锁
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read1.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read2.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read3.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read4.getTransactionID() ) );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList );

        // 提交事务1
        db1.commit();

        // 查询线程判断返回成功，且不再等锁
        Assert.assertTrue( read1.isSuccess(), read1.getErrorMsg() );
        Assert.assertTrue( read2.isSuccess(), read2.getErrorMsg() );
        Assert.assertTrue( read3.isSuccess(), read3.getErrorMsg() );
        Assert.assertTrue( read4.isSuccess(), read4.getErrorMsg() );

        // 再次事务中查询
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expList );
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}", expList );

        // 非事务查询
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList );

        // 提交所有事务
        db2.commit();
        db3.commit();
        db4.commit();
        db5.commit();
    }

    private class Query extends SdbThreadBase {
        private String hint;
        private List< BSONObject > expList;
        private DBCollection cl;
        private String findConf;

        private Query( DBCollection cl, String findConf, String hint,
                List< BSONObject > expList ) {
            this.cl = cl;
            this.findConf = findConf;
            this.hint = hint;
            this.expList = expList;
        }

        @Override
        public void exec() throws Exception {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl.getSequoiadb() );

            TransUtils.queryAndCheck( cl, findConf, "", "{a:1}", hint,
                    expList );
        }
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        db3.commit();
        db4.commit();
        db5.commit();
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
        }
        if ( !db4.isClosed() ) {
            db4.close();
        }
        if ( !db5.isClosed() ) {
            db5.close();
        }
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }
}
