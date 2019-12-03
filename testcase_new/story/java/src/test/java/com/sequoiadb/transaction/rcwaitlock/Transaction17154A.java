package com.sequoiadb.transaction.rcwaitlock;

/**
 * @Description seqDB-17154: 更新记录与读记录并发，事务提交 
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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rcwaitlock")
public class Transaction17154A extends SdbTestBase {
    private String clName = "cl_17154A";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCursor cursor = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        cl.insert( "{_id:1, a:1, b:1}" );
    }

    @Test
    public void test() {
        // 开启事务1及事务2
        db1.beginTransaction();

        // 事务1更新索引字段的值
        cl1.update( null, "{$set:{a:2}}", "{'':'a'}" );
        BSONObject updateR1 = ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" );
        expList.add( updateR1 );

        // 事务2表扫描记录，匹配条件使用更新后值
        Read read1 = new Read( "{a:2}", "{'':null}", expList );
        read1.start();
        Assert.assertTrue( read1.matchBlockingMethod( DBCursor.class.getName(),
                "hasNext" ) );

        // 事务2索引扫描记录，匹配条件使用更新后值
        Read read2 = new Read( "{a:2}", "{'':'a'}", expList );
        read2.start();
        Assert.assertTrue( read2.matchBlockingMethod( DBCursor.class.getName(),
                "hasNext" ) );

        // 事务3记录读，匹配条件使用更新前值
        Read read3 = new Read( "{a:1}", "{'':null}",
                new ArrayList< BSONObject >() );
        read3.start();
        Assert.assertTrue( read3.matchBlockingMethod( DBCursor.class.getName(),
                "hasNext" ) );

        // 事务3索引读，匹配条件使用更新前值
        Read read4 = new Read( "{a:1}", "{'':'a'}",
                new ArrayList< BSONObject >() );
        read4.start();
        Assert.assertTrue( read4.matchBlockingMethod( DBCursor.class.getName(),
                "hasNext" ) );

        // 非事务表扫描记录
        cursor = cl.query( null, null, null, "{'':null}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        // 非事务索引扫描记录
        cursor = cl.query( null, null, null, "{'':'a'}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        db1.commit();

        // 校验阻塞线程返回的记录
        if ( !read1.isSuccess() || !read2.isSuccess() || !read3.isSuccess()
                || !read4.isSuccess() ) {
            Assert.fail( read1.getErrorMsg() + read2.getErrorMsg()
                    + read3.getErrorMsg() + read4.getErrorMsg() );
        }
        try {
            Assert.assertEquals( read1.getExecResult(), expList );
            Assert.assertEquals( read2.getExecResult(), expList );
            Assert.assertEquals( read3.getExecResult(),
                    new ArrayList< BSONObject >() );
            Assert.assertEquals( read4.getExecResult(),
                    new ArrayList< BSONObject >() );
        } catch ( Exception e ) {
            Assert.fail( e.getMessage() );
        }

        cursor.close();
    }

    private class Read extends SdbThreadBase {
        private Sequoiadb db = null;
        private Sequoiadb db2 = null;
        private DBCollection cl = null;
        private DBCollection cl2 = null;
        private String findConf = null;
        private String hint = null;
        private DBCursor cursor = null;
        private List< BSONObject > expScanList = new ArrayList< BSONObject >();

        public Read( String findConf, String hint,
                List< BSONObject > expScanList ) {
            this.hint = hint;
            this.findConf = findConf;
            this.expScanList = expScanList;

        }

        @Override
        public void exec() throws Exception {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );

            // 开启并发事务2
            db2.beginTransaction();

            try {
                cursor = cl2.query( findConf, null, null, hint );
                List< BSONObject > records = TransUtils
                        .getReadActList( cursor );
                setExecResult( records );

                // 事务2扫描记录
                cursor = cl2.query( findConf, null, null, hint );
                Assert.assertEquals( TransUtils.getReadActList( cursor ),
                        expScanList );

                // 非事务扫描记录
                cursor = cl.query( findConf, null, null, hint );
                Assert.assertEquals( TransUtils.getReadActList( cursor ),
                        expScanList );

                db2.commit();
            } finally {
                db2.commit();
                cursor.close();
                db2.close();
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        if ( !db1.isClosed() ) {
            db1.close();
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
