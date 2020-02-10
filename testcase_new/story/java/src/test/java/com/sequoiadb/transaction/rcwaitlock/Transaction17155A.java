package com.sequoiadb.transaction.rcwaitlock;

/**
 * @Description seqDB-17155: 更新记录与读记录并发，事务回滚 
 * @author xiaoni Zhao
 * @date 2019-1-22
 */
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

@Test(groups = "rcwaitlock")
public class Transaction17155A extends SdbTestBase {
    private String clName = "cl_17155A";
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
    private List< BSONObject > expList1 = new ArrayList< BSONObject >();
    private List< BSONObject > expList2 = new ArrayList< BSONObject >();

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
    }

    @Test
    public void test() throws InterruptedException {
        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        cl.insert( insertR1 );
        expList1.add( insertR1 );

        // 开启事务1及事务2
        db1.beginTransaction();
        db2.beginTransaction();
        db3.beginTransaction();
        db4.beginTransaction();
        db5.beginTransaction();

        // 事务1更新索引字段的值
        cl1.update( null, "{$set:{a:2}}", "{'':'a'}" );
        BSONObject updateR1 = ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" );
        expList2.add( updateR1 );

        // 事务匹配条件使用更新后值
        Query read1 = new Query( cl2, "{a:2}", "{'':null}",
                new ArrayList< BSONObject >() );
        read1.start();
        Query read2 = new Query( cl3, "{a:2}", "{'':'a'}",
                new ArrayList< BSONObject >() );
        read2.start();

        // 事务匹配条件使用更新前值
        Query read3 = new Query( cl4, "{a:1}", "{'':null}", expList1 );
        read3.start();
        Query read4 = new Query( cl5, "{a:1}", "{'':'a'}", expList1 );
        read4.start();

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList2 );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList2 );

        // 回滚事务1
        db1.rollback();

        // 查询线程判断返回成功，且不再等锁
        Assert.assertTrue( read1.isSuccess(), read1.getErrorMsg() );
        Assert.assertTrue( read2.isSuccess(), read2.getErrorMsg() );

        Assert.assertTrue( read3.isSuccess(), read3.getErrorMsg() );
        Assert.assertTrue( read4.isSuccess(), read4.getErrorMsg() );

        // 再次事务中查询
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expList1 );
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}", expList1 );

        // 非事务查询
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList1 );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList1 );

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
