package com.sequoiadb.crud;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * test content: 插入与查询并发 testlink-case: seqDB-11427
 * 
 * @author wangkexin
 * @Date 2019.03.15
 * @version 1.00
 */
public class InsertAndQuery11427 extends SdbTestBase {// TODO:没有按评审意见修改，请修改
    private String clName = "cl11427";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private Set< BSONObject > expectData = new HashSet<>();

    @BeforeClass
    public void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clName );
        insertRecords( cl );
    }

    @Test
    private void test() {
        QueryThread queryThread = new QueryThread();
        InsertThread insertThread = new InsertThread();

        queryThread.start();
        insertThread.start();

        Assert.assertTrue( queryThread.isSuccess(), queryThread.getErrorMsg() );
        Assert.assertTrue( insertThread.isSuccess(),
                insertThread.getErrorMsg() );

        checkRecords( cl );
    }

    @AfterClass
    public void teardown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

    private class InsertThread extends SdbThreadBase {
        @Override
        public void exec() {
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                insertRecords( cl );
            }
        }
    }

    private class QueryThread extends SdbThreadBase {
        private int count = 0;

        @Override
        public void exec() {
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject matcher = ( BSONObject ) JSON
                        .parse( "{age:{$gte:10}}" );
                BSONObject orderBy = ( BSONObject ) JSON.parse( "{age:1}" );
                DBCursor queryCursor = cl.query( matcher, null, orderBy, null );
                while ( queryCursor.hasNext() ) {
                    queryCursor.getNext();
                    count++;
                }
                if ( count < 90 ) {
                    Assert.fail(
                            "unexpected query result, expect returned num >= "
                                    + 90 + " , actual returned num : "
                                    + count );
                }
            }
        }
    }

    private void insertRecords( DBCollection cl ) {
        int normalRecNum = 100;
        ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
        BSONObject record = new BasicBSONObject();
        for ( int i = 0; i < normalRecNum; i++ ) {
            record = new BasicBSONObject();
            record.put( "name", "zhangsan" + i );
            record.put( "age", i );
            record.put( "num", i );
            insertRecords.add( record );
        }
        cl.insert( insertRecords );
        expectData.addAll( insertRecords );
    }

    private void checkRecords( DBCollection cl ) {
        Set< BSONObject > actualData = new HashSet<>();
        DBCursor queryCursor = cl.query();
        while ( queryCursor.hasNext() ) {
            actualData.add( queryCursor.getNext() );
        }
        Assert.assertEquals( expectData, actualData );
    }
}
