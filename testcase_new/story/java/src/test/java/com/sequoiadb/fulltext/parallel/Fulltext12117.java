package com.sequoiadb.fulltext.parallel;

import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-12117:同一集合并发创建不同的全文索引
 * @Author yinzhen
 * @Date 2019-4-28
 */
public class Fulltext12117 extends FullTestBase {
    private String clName = "cl12117";
    private String fullIdxName;
    private String esIndexName;
    private String cappedCLName;
    private AtomicInteger atoint = new AtomicInteger( 0 );
    private int insertNum = 20000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() {
        FullTextDBUtils.insertData( cl, insertNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        for ( int i = 0; i < 10; i++ ) {
            thExecutor.addWorker( new CreateFullIdx() );
        }
        thExecutor.run();
        Assert.assertEquals( atoint.get(), 1 );

        // 主备节点上索引信息及固定集合中数据正确且一致，ES上同步的索引数据正确，全文检索结果正确
        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIdxName );
        cappedCLName = FullTextDBUtils.getCappedName( cl, fullIdxName );
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, fullIdxName, insertNum ) );

        // Java 驱动，一个连接只有一个收缓存区和一个发缓存区，收发需要加锁，因此需要定义两个连接
        Sequoiadb db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor dbCursor = cl.query( "{}", "{}", "{_id:1}", "{}" );
            DBCursor esCursor = cl2.query(
                    "{'':{'$Text':{'query':{'match_all':{}}}}}", "{}",
                    "{_id:1}", "{'':'" + fullIdxName + "'}" );
            Assert.assertTrue( FullTextUtils.isCLRecordsConsistency( dbCursor,
                    esCursor ) );
        } finally {
            if ( db2 != null ) {
                db2.close();
            }
        }

        // 在db端执行插入、全文检索
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            FullTextDBUtils.insertData( cl, 1000 );
            Assert.assertEquals( cl.getCount(), insertNum + 1000 );
            Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIdxName,
                    insertNum + 1000 ) );

            DBCollection cl3 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor dbCursor = cl.query( "{}", "{}", "{_id:1}", "{}" );
            DBCursor esCursor = cl3.query(
                    "{'':{'$Text':{'query':{'match_all':{}}}}}", "{}",
                    "{_id:1}", "{'':'" + fullIdxName + "'}" );
            Assert.assertTrue( FullTextUtils.isCLRecordsConsistency( dbCursor,
                    esCursor ) );
        } finally {
            if ( db2 != null ) {
                db2.close();
            }
        }

        // 全文检索校验
        FullTextUtils.isRecordEqualsByMulQueryMode( cl );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
    }

    private class CreateFullIdx {
        @ExecuteOrder(step = 1, desc = "多线程并发创建不同的全文索引")
        private void createFullIdx() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                String idxName = "idx12117" + Thread.currentThread().getName();
                cl.createIndex( idxName,
                        "{'a':'text','b':'text','c':'text', 'd':'text', 'e':'text', 'f':'text'}",
                        false, false );
                fullIdxName = idxName;
                atoint.incrementAndGet();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -42 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }
}