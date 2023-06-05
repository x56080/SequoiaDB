package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-31537 :: 并发创建全文索引和插入数据
 * @author wuyan
 * @date 2023.5.24
 * @version 1.10
 */
public class Fulltext31537 extends FullTestBase {
    private boolean runSuccess = false;
    private String clName = "fulltext_cl31537";
    private String fullIdxName = "fulltext_31537";
    private int insertNum = 20000;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thExecutor.addWorker( new CreateFullIndex() );
        thExecutor.addWorker( new InsertData() );

        thExecutor.run();

        // 原集合、固定集合中记录正确且主备节点数据一致，ES中最终同步的记录正确
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIdxName,
                ( int ) cl.getCount() ) );

        // Java 驱动，一个连接只有一个收缓存区和一个发缓存区，收发需要加锁，因此需要定义两个连接
        try ( Sequoiadb db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor dbCursor = cl.query( "{}", "{}", "{_id:1}", "{}" );
            DBCursor esCursor = cl2.query(
                    "{'':{'$Text':{'query':{'match_all':{}}}}}", "{}",
                    "{_id:1}", "{'':'" + fullIdxName + "'}" );
            Assert.assertTrue( FullTextUtils.isCLRecordsConsistency( dbCursor,
                    esCursor ) );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateFullIndex {
        BSONObject indexKeys = new BasicBSONObject();
        BSONObject indexAttr = new BasicBSONObject();
        BSONObject mappings = new BasicBSONObject();
        BSONObject fields = new BasicBSONObject();
        BSONObject keyType1 = new BasicBSONObject();
        BSONObject keyType2 = new BasicBSONObject();

        public CreateFullIndex() {
            keyType1.put( "Type", "long" );
            keyType2.put( "Type", "keyword" );
            fields.put( "no", keyType1 );
            fields.put( "tobj.test", keyType2 );
            mappings.put( "Fields", fields );
            indexAttr.put( "Mappings", mappings );
            indexKeys.put( "no", "text" );
            indexKeys.put( "tobj.test", "text" );
        }

        @ExecuteOrder(step = 1, desc = "创建全文索引")
        private void createFullIndex() {

            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createIndex( fullIdxName, indexKeys, indexAttr );
            }
        }
    }

    private class InsertData {
        @ExecuteOrder(step = 1, desc = "插入包含全文索引字段的记录")
        private void insertRecords() {
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl, insertNum );
            }
        }
    }

    private void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                BSONObject record = new BasicBSONObject();
                BSONObject objValue = new BasicBSONObject();
                objValue.put( "test", StringUtils.getRandomString( i + 5 ) );
                record.put( "no", i );
                record.put( "tobj", objValue );
                records.add( record );
            }
            cl.bulkInsert( records );
            records.clear();
        }
    }
}