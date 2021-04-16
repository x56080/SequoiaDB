package com.mongodb.java.concurrent;

import static com.mongodb.client.model.Filters.and;
import static com.mongodb.client.model.Filters.eq;
import static com.mongodb.client.model.Filters.exists;
import static com.mongodb.client.model.Filters.gte;
import static com.mongodb.client.model.Filters.lt;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import org.bson.Document;
import org.bson.conversions.Bson;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.MongoCommandException;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Filters;
import com.mongodb.client.model.IndexOptions;
import com.mongodb.client.model.Indexes;
import com.mongodb.client.model.UpdateOptions;
import com.mongodb.client.model.Updates;
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
    private MongoDatabase db;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        if ( client.listDatabaseNames().into( new ArrayList<>() )
                .contains( dbName ) ) {
            client.getDatabase( dbName ).drop();
        }
        db = client.getDatabase( dbName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        int threadNum = 10;
        for ( int i = 0; i < threadNum; i++ ) {
            threadExec.addWorker( new CreateCSCL() );
            threadExec.addWorker(
                    new Insert( Arrays.asList( new Document( "a", i ),
                            new Document( "a", i + threadNum ) ) ) );
            threadExec.addWorker( new CreateIndex( "a" + i ) );
            threadExec.addWorker(
                    new Upsert( Filters.and( eq( "a", i + 2 * threadNum ) ),
                            Updates.set( "b", i ) ) );
        }
        threadExec.run();
        // 简单检查
        MongoCollection< Document > cl = db.getCollection( clName );
        Collection< Document > indexInfos = cl.listIndexes()
                .into( new ArrayList< Document >() );
        Assert.assertEquals( indexInfos.size(), threadNum + 1,
                indexInfos.toString() );
        Assert.assertEquals(
                cl.count( and( gte( "a", 0 ), lt( "a", 3 * threadNum ) ) ),
                3 * threadNum );
        Assert.assertEquals(
                cl.count( and( gte( "a", 2 * threadNum ),
                        lt( "a", 3 * threadNum ), exists( "b", true ) ) ),
                threadNum );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        if ( runSuccess )
            db.drop();
    }

    private class CreateCSCL {
        @ExecuteOrder(step = 1)
        private void create() {
            MongoDatabase db = client.getDatabase( dbName );
            try {
                db.createCollection( clName );
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
            MongoDatabase db = client.getDatabase( dbName );
            MongoCollection< Document > cl = db.getCollection( clName );
            cl.createIndex( Indexes.ascending( keyAndName ),
                    new IndexOptions().unique( false ).name( keyAndName ) );
        }
    }

    private class Insert {
        private List< Document > documentList;

        public Insert( List< Document > documentList ) {
            this.documentList = documentList;
        }

        @ExecuteOrder(step = 1)
        private void insert() {
            MongoDatabase db = client.getDatabase( dbName );
            MongoCollection< Document > cl = db.getCollection( clName );
            cl.insertMany( documentList );
        }
    }

    private class Upsert {
        private Bson query;
        private Bson update;

        public Upsert( Bson query, Bson update ) {
            this.query = query;
            this.update = update;
        }

        @ExecuteOrder(step = 1)
        private void upsert() {
            MongoDatabase db = client.getDatabase( dbName );
            MongoCollection< Document > cl = db.getCollection( clName );
            cl.updateMany( query, update, new UpdateOptions().upsert( true ) );
        }
    }
}
