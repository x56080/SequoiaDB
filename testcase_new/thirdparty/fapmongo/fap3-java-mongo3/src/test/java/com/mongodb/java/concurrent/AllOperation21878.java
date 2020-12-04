package com.mongodb.java.concurrent;

import static com.mongodb.client.model.Filters.eq;
import static com.mongodb.client.model.Filters.gte;
import static com.mongodb.client.model.Filters.lt;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;

import org.bson.Document;
import org.bson.conversions.Bson;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Accumulators;
import com.mongodb.client.model.Aggregates;
import com.mongodb.client.model.Filters;
import com.mongodb.client.model.IndexOptions;
import com.mongodb.client.model.Indexes;
import com.mongodb.client.model.Sorts;
import com.mongodb.client.model.UpdateOptions;
import com.mongodb.client.model.Updates;
import com.mongodb.client.result.DeleteResult;
import com.mongodb.client.result.UpdateResult;
import com.mongodb.utils.MongodbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-21878:并发做CRUD+创建/删除索引操作
 * @author fanyu
 * @Date 2020/4/1
 * @version 1.00
 */
public class AllOperation21878 extends MongodbTestBase {
    private String clName = "cl21878";
    private int threadNumPerOpera = 5;
    private AtomicLong totalDelNum = new AtomicLong( 0 );
    private AtomicLong totalUpateNum = new AtomicLong( 0 );

    @BeforeClass
    public void setUp() throws UnknownHostException {
        MongoDatabase db = MongodbTestBase.getDataBase( client );
        MongoCollection< Document > cl = db.getCollection( clName );
        cl.createIndex( Indexes.ascending( "a" ),
                new IndexOptions().unique( false ).name( "a" ) );
        List< Document > list = new ArrayList<>();
        for ( int i = 0; i < threadNumPerOpera * threadNumPerOpera; i++ ) {
            list.add(
                    new Document( "a", i ).append( "b", String.valueOf( i ) ) );
            list.add(
                    new Document( "c", i ).append( "d", String.valueOf( i ) ) );
        }
        cl.insertMany( list );
    }

    @Test(enabled = false) // jira-6463
    public void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new CreateIndex( "a1" ) );
        threadExec.addWorker( new DeleteIndex( "a" ) );
        for ( int i = 0; i < threadNumPerOpera; i++ ) {
            // insert
            threadExec.addWorker( new Insert( Arrays.asList(
                    new Document( "a1", threadNumPerOpera + i ),
                    new Document( "b1",
                            String.valueOf( threadNumPerOpera + i ) ) ) ) );

            // find
            threadExec.addWorker( new Find( new Document() ) );

            // upsert [threadNumPerOpera*threadNumPerOpera,
            threadExec.addWorker( new Upsert(
                    Filters.and( eq( "a2", i + threadNumPerOpera ) ),
                    Updates.set( "b2",
                            String.valueOf( threadNumPerOpera + i ) ) ) );

            // update and delete
            // [0 threadNumPerOpera*threadNumPerOpera]
            Bson query1 = Filters.and( gte( "a", i * threadNumPerOpera ),
                    lt( "a", ( i + 1 ) * threadNumPerOpera ) );

            threadExec.addWorker( new Delete( query1 ) );

            threadExec.addWorker( new Update( query1, Updates.combine(
                    Updates.set( "a",
                            i + threadNumPerOpera * threadNumPerOpera ),
                    Updates.set( "e",
                            i + threadNumPerOpera * threadNumPerOpera ) ) ) );

            // count
            Bson query2 = Filters.and( gte( "c", i * threadNumPerOpera ),
                    lt( "c", ( i + 1 ) * threadNumPerOpera ) );
            threadExec.addWorker( new Count( query2 ) );

            // aggregate
            List< Bson > agg = Arrays.asList( Aggregates.match( query2 ),
                    Aggregates.group( "$d", Accumulators.sum( "sum_c", "$c" ) ),
                    Aggregates.sort( Sorts.ascending( "_id" ) ) );
            threadExec.addWorker( new Aggregate( agg, new int[] {
                    i * threadNumPerOpera, ( i + 1 ) * threadNumPerOpera } ) );

            // distinct
            threadExec.addWorker( new Distinct( "c", query2, new int[] {
                    i * threadNumPerOpera, ( i + 1 ) * threadNumPerOpera } ) );
        }
        threadExec.run();

        // 更新的个数 = threadNumPerOpera * threadNumPerOpera - totalDelNum.get()
        MongoDatabase db = MongodbTestBase.getDataBase( client );
        MongoCollection< Document > cl = db.getCollection( clName );
        Bson query = Filters.and( Filters.exists( "e" ), gte( "e", -1 ) );
        Assert.assertEquals( cl.count( query ),
                threadNumPerOpera * threadNumPerOpera - totalDelNum.get() );
        Assert.assertEquals( cl.count( query ), totalUpateNum.get() );
    }

    @AfterClass
    public void tearDown() {
    }

    private class CreateIndex {
        private String keyAndName;

        public CreateIndex( String keyAndName ) {
            this.keyAndName = keyAndName;
        }

        @ExecuteOrder(step = 1)
        private void create() {
            MongoDatabase db = MongodbTestBase.getDataBase( client );
            MongoCollection< Document > cl = db.getCollection( clName );
            cl.createIndex( Indexes.ascending( keyAndName ),
                    new IndexOptions().unique( false ).name( keyAndName ) );
        }
    }

    private class Find {
        private Bson query;

        public Find( Bson query ) {
            this.query = query;
        }

        @ExecuteOrder(step = 1)
        private void create() {
            MongoDatabase db = MongodbTestBase.getDataBase( client );
            MongoCollection< Document > cl = db.getCollection( clName );
            cl.find( query );
        }
    }

    private class DeleteIndex {
        private String name;

        public DeleteIndex( String name ) {
            this.name = name;
        }

        @ExecuteOrder(step = 1)
        private void create() {
            MongoDatabase db = MongodbTestBase.getDataBase( client );
            MongoCollection< Document > cl = db.getCollection( clName );
            cl.dropIndex( name );
        }
    }

    private class Insert {
        private List< Document > documentList;

        public Insert( List< Document > documentList ) {
            this.documentList = documentList;
        }

        @ExecuteOrder(step = 1)
        private void insert() {
            MongoDatabase db = MongodbTestBase.getDataBase( client );
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
            MongoDatabase db = MongodbTestBase.getDataBase( client );
            MongoCollection< Document > cl = db.getCollection( clName );
            cl.updateMany( query, update, new UpdateOptions().upsert( true ) );
        }
    }

    private class Update {
        private Bson query;
        private Bson update;

        public Update( Bson query, Bson update ) {
            this.query = query;
            this.update = update;
        }

        @ExecuteOrder(step = 1)
        private void upsert() {
            MongoDatabase db = MongodbTestBase.getDataBase( client );
            MongoCollection< Document > cl = db.getCollection( clName );
            UpdateResult result = cl.updateMany( query, update,
                    new UpdateOptions().upsert( false ) );
            totalUpateNum.getAndAdd( result.getModifiedCount() );
        }
    }

    private class Delete {
        private Bson query;

        public Delete( Bson query ) {
            this.query = query;
        }

        @ExecuteOrder(step = 1)
        private void delete() {
            MongoDatabase db = MongodbTestBase.getDataBase( client );
            MongoCollection< Document > cl = db.getCollection( clName );
            DeleteResult result = cl.deleteMany( query );
            totalDelNum.getAndAdd( result.getDeletedCount() );
        }
    }

    private class Count {
        private Bson query;

        public Count( Bson query ) {
            this.query = query;
        }

        @ExecuteOrder(step = 1)
        private void count() {
            MongoDatabase db = MongodbTestBase.getDataBase( client );
            MongoCollection< Document > cl = db.getCollection( clName );
            long count = cl.count( query );
            Assert.assertEquals( count, threadNumPerOpera );
        }
    }

    private class Distinct {
        private String filedName;
        private Bson query;
        private int[] checkResult;

        public Distinct( String filedName, Bson query, int[] checkResult ) {
            this.filedName = filedName;
            this.query = query;
            this.checkResult = checkResult;
        }

        @ExecuteOrder(step = 1)
        private void distinct() {
            MongoDatabase db = MongodbTestBase.getDataBase( client );
            MongoCollection< Document > cl = db.getCollection( clName );
            List< Integer > result = ( List< Integer > ) cl
                    .distinct( filedName, query, Integer.class )
                    .into( new ArrayList< Integer >() );
            List< Integer > checkList = new ArrayList<>();
            for ( int i = checkResult[ 0 ]; i < checkResult[ 1 ]; i++ ) {
                checkList.add( i );
            }
            Assert.assertEquals( result, checkList );
        }
    }

    private class Aggregate {
        private List< Bson > agg;
        private int[] checkResult;

        public Aggregate( List< Bson > agg, int[] checkResult ) {
            this.agg = agg;
            this.checkResult = checkResult;
        }

        @ExecuteOrder(step = 1)
        private void aggregate() {
            MongoDatabase db = MongodbTestBase.getDataBase( client );
            MongoCollection< Document > cl = db.getCollection( clName );
            Collection< Document > result = cl.aggregate( agg )
                    .into( new ArrayList< Document >() );
            List< Document > checkList = new ArrayList<>();
            for ( int i = checkResult[ 0 ]; i < checkResult[ 1 ]; i++ ) {
                checkList.add( new Document( "_id", i ).append( "sum_c",
                        ( double ) i ) );
            }
            Assert.assertEquals( result.toString(), checkList.toString() );
        }
    }
}
