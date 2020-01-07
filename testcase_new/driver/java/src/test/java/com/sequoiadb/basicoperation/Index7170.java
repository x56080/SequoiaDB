package com.sequoiadb.basicoperation;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Index7165.java
 * TestLink: seqDB-7170
 * 
 * @author zhaoyu
 * @Date 2016.9.22
 * @version 1.00
 */
public class Index7170 extends SdbTestBase {
    private Sequoiadb sdb;
    private DBCollection cl = null;
    private String clName = "cl7170";
    private CollectionSpace cs = null;
    private SimpleDateFormat df = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss.SSS" );
    private String coordAddr;
    private String commCSName;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.commCSName = SdbTestBase.csName;
        try {
            System.out.println( "the TestCase: " + this.getClass().getName()
                    + " begin at:" + df.format( new Date().getTime() ) );
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( !sdb.isCollectionSpaceExist( commCSName ) ) {
                sdb.createCollectionSpace( commCSName );
            }
            this.cs = this.sdb.getCollectionSpace( commCSName );

            // create cl
            try {
                if ( this.cs.isCollectionExist( clName ) ) {
                    this.cs.dropCollection( clName );
                }
                this.cl = this.cs.createCollection( clName );
            } catch ( BaseException e ) {
                Assert.fail( "create cl:" + clName + " failed, errMsg:"
                        + e.getMessage() );
            }

            // insert data
            int dataNum = 10;
            ArrayList< BSONObject > insertData = new ArrayList< BSONObject >();
            try {
                for ( int i = 0; i < dataNum; i++ ) {
                    BSONObject dataObj = new BasicBSONObject();
                    dataObj.put( "a", i );
                    dataObj.put( "b", i );
                    insertData.add( dataObj );
                }
                this.cl.bulkInsert( insertData, 0 );
            } catch ( BaseException e ) {
                Assert.fail( "insert data:" + insertData + " failed, errMsg:"
                        + e.getMessage() );
            }
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            System.out.println( "the TestCase: " + this.getClass().getName()
                    + " end at:" + df.format( new Date().getTime() ) );
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test(invocationCount = 10, threadPoolSize = 5)
    public void test() {
        Sequoiadb sdbConcurrency = null;
        try {
            sdbConcurrency = new Sequoiadb( coordAddr, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( "connect sdbConcurrency failed" + e.getMessage() );
        }

        CollectionSpace csConcurrency = sdbConcurrency
                .getCollectionSpace( commCSName );
        DBCollection clConcurrency = csConcurrency.getCollection( clName );
        int dataNum = 100;
        createIndex( clConcurrency, dataNum );
        insertData( clConcurrency, dataNum );
        updateData( clConcurrency, dataNum );
        deleteData( clConcurrency, dataNum );
        sdbConcurrency.disconnect();
    }

    public void createIndex( DBCollection cl, int dataNum ) {
        BSONObject indexOption = ( BSONObject ) JSON.parse( "{a:1}" );
        String indexName = null;
        boolean isUnique = false;
        boolean enforced = false;
        try {
            for ( int i = 0; i < dataNum; i++ ) {
                indexName = "aIndex";
                cl.createIndex( indexName, indexOption, isUnique, enforced, 0 );
                cl.dropIndex( indexName );
            }
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -247 ) {
                Assert.fail( "create or drop index name : " + indexName
                        + " failed, errMsg:" + e.getMessage() );
            }
        }
    }

    public void insertData( DBCollection cl, int dataNum ) {
        BSONObject dataObj = null;
        try {
            for ( int i = 0; i < dataNum; i++ ) {
                dataObj = new BasicBSONObject();
                dataObj.put( "a", i );
                dataObj.put( "b", i );
                cl.insert( dataObj );
            }
        } catch ( BaseException e ) {
            Assert.fail( "insert data:" + dataObj + " failed, errMsg:"
                    + e.getMessage() );
        }
    }

    public void updateData( DBCollection cl, int dataNum ) {
        BSONObject modifier = null;
        BSONObject modifyObj = null;
        BSONObject match = null;
        try {
            for ( int i = 0; i < dataNum; i++ ) {
                modifier = new BasicBSONObject();
                modifyObj = new BasicBSONObject();
                match = new BasicBSONObject();
                modifyObj.put( "a", i - 1 );
                modifier.put( "$set", modifyObj );
                cl.update( match, modifier, null );
            }
        } catch ( BaseException e ) {
            Assert.fail( "update data:" + modifier + " failed, errMsg:"
                    + e.getMessage() );
        }
    }

    public void deleteData( DBCollection cl, int dataNum ) {
        BSONObject match = null;
        try {
            for ( int i = 0; i < dataNum; i++ ) {
                match = new BasicBSONObject();
                cl.delete( match );
            }
        } catch ( BaseException e ) {
            Assert.fail( "delete data:" + match + " failed, errMsg:"
                    + e.getMessage() );
        }
    }

}
