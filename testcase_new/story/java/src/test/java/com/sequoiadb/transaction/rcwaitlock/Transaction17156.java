package com.sequoiadb.transaction.rcwaitlock;

import java.util.ArrayList;
/**
 * @Description seqDB-17156:  删除记录与读记录并发，事务提交 
 * @author xiaoni Zhao
 * @date 2019-1-22
 */
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
public class Transaction17156 extends SdbTestBase {
    private String clName = "cl_17156";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;

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
        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        cl.insert( insertR1 );
    }

    @Test
    public void test() throws InterruptedException {
        // 开启事务1
        db1.beginTransaction();
        db2.beginTransaction();
        db3.beginTransaction();

        // 事务1删除记录
        cl1.delete( null, "{'':'a'}" );

        // 事务查询
        Query read1 = new Query( cl2, "{'':null}",
                new ArrayList< BSONObject >() );
        read1.start();
        Query read2 = new Query( cl3, "{'':'a'}",
                new ArrayList< BSONObject >() );
        read2.start();

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}",
                new ArrayList< BSONObject >() );

        // 提交事务1
        db1.commit();

        // 查询线程判断返回成功，且不再等锁
        Assert.assertTrue( read1.isSuccess(), read1.getErrorMsg() );
        Assert.assertTrue( read2.isSuccess(), read2.getErrorMsg() );

        // 再次事务中查询
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}",
                new ArrayList< BSONObject >() );

        // 非事务查询
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}",
                new ArrayList< BSONObject >() );

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
