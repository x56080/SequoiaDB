package com.mongodb.java.concurrent;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.MongoCommandException;
import com.mongodb.QueryBuilder;
import com.mongodb.utils.MongodbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-21998:并发自动创建cs和cl
 * @author fanyu
 * @Date 2020/4/1
 * @version 1.00
 */
public class AutoCreateCSCL21998 extends MongodbTestBase {
    private boolean runSuccess = false;
    private String dbName = "db21998";
    private String clName = "cl21998";
    private DB db;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = client.getDB( dbName );
        db.dropDatabase();
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        int threadNum = 10;
        for ( int i = 0; i < threadNum; i++ ) {
            threadExec.addWorker( new CreateCSCL() );
            List< DBObject > list = new ArrayList<>();
            list.add( new BasicDBObject( "a", i ) );
            list.add( new BasicDBObject( "a", i + threadNum ) );
            threadExec.addWorker( new Insert( list ) );
            threadExec.addWorker( new CreateIndex( "a" + i ) );
            DBObject query = QueryBuilder.start( "a" ).is( i + 2 * threadNum )
                    .get();
            DBObject update = new BasicDBObject( "$set",
                    new BasicDBObject( "b", i ) );
            threadExec.addWorker( new Upsert( query, update ) );
        }
        threadExec.run();
        // 简单检查
        DBCollection cl = db.getCollection( clName );
        List< DBObject > indexInfos = cl.getIndexInfo();
        Assert.assertEquals( indexInfos.size(), threadNum + 1,
                indexInfos.toString() );

        DBObject query = QueryBuilder.start( "a" ).greaterThanEquals( 0 )
                .lessThan( 3 * threadNum ).get();
        Assert.assertEquals( cl.count( query ), 3 * threadNum );

        query = QueryBuilder.start( "a" ).greaterThanEquals( 2 * threadNum )
                .lessThan( 3 * threadNum ).and( "b" ).exists( true ).get();
        Assert.assertEquals( cl.count( query ), threadNum );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        if ( runSuccess ) {
            db.dropDatabase();
        }
    }

    private class CreateCSCL {
        @ExecuteOrder(step = 1)
        private void create() {
            DB db = client.getDB( dbName );
            try {
                db.createCollection( clName, new BasicDBObject() );
            } catch ( MongoCommandException e ) {
                if ( e.getErrorCode() != -22 ) {
                    throw e;
                }
            }
        }
    }

    private class CreateIndex {
        private String keyAndName;

        public CreateIndex( String keyAndName ) {
            this.keyAndName = keyAndName;
        }

        @ExecuteOrder(step = 1)
        private void create() {
            DB db = client.getDB( dbName );
            DBCollection cl = db.getCollection( clName );
            cl.createIndex( new BasicDBObject( keyAndName, 1 ), keyAndName,
                    false );
        }
    }

    private class Insert {
        private List< DBObject > documentList;

        public Insert( List< DBObject > documentList ) {
            this.documentList = documentList;
        }

        @ExecuteOrder(step = 1)
        private void insert() {
            DB db = client.getDB( dbName );
            DBCollection cl = db.getCollection( clName );
            cl.insert( documentList );
        }
    }

    private class Upsert {
        private DBObject query;
        private DBObject update;

        public Upsert( DBObject query, DBObject update ) {
            this.query = query;
            this.update = update;
        }

        @ExecuteOrder(step = 1)
        private void upsert() {
            DB db = client.getDB( dbName );
            DBCollection cl = db.getCollection( clName );
            cl.update( query, update, true, false );
        }
    }
}
