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
public class Transaction17155C extends SdbTestBase {
    private String clName = "cl_17155C";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private List< BSONObject > expList1 = new ArrayList< BSONObject >();
    private List< BSONObject > expList2 = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl3 = db3.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @Test
    public void test() throws InterruptedException {
        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        cl.insert( insertR1 );
        expList1.add( insertR1 );

        // 开启事务1
        db1.beginTransaction();
        db2.beginTransaction();
        db3.beginTransaction();

        // 判断事务阻塞需先获取事务id
        String transactionID2 = TransUtils.getTransactionID( db2 );
        String transactionID3 = TransUtils.getTransactionID( db3 );

        // 事务1删除索引字段的值
        BSONObject updateR1 = ( BSONObject ) JSON.parse( "{_id:1, b:1}" );
        cl1.update( null, "{$unset:{a:1}}", "{'':'a'}" );
        expList2.add( updateR1 );

        // 事务查询
        Query read1 = new Query( cl2, "{'':null}", expList1 );
        read1.start();
        Query read2 = new Query( cl3, "{'':'a'}", expList1 );
        read2.start();

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList2 );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList2 );

        // 提交事务1
        db1.rollback();

        // 查询线程判断返回成功，且不再等锁
        Assert.assertTrue( read1.isSuccess(), read1.getErrorMsg() );
        Assert.assertTrue( read2.isSuccess(), read2.getErrorMsg() );

        Assert.assertFalse( TransUtils.isTransWaitLock( sdb, transactionID2 ) );
        Assert.assertFalse( TransUtils.isTransWaitLock( sdb, transactionID3 ) );

        // 再次事务中查询
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expList1 );
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}", expList1 );

        // 非事务查询
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList1 );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList1 );

        // 提交所有事务
        db2.commit();
        db3.commit();
    }

    private class Query extends SdbThreadBase {
        private String hint;
        private List< BSONObject > expList;
        private DBCollection cl;

        private Query( DBCollection cl, String hint,
                List< BSONObject > expList ) {
            this.cl = cl;
            this.hint = hint;
            this.expList = expList;
        }

        @Override
        public void exec() throws Exception {
            TransUtils.queryAndCheck( cl, "{a:1}", hint, expList );
        }
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        db3.commit();
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
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
