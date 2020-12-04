
package com.mongodb.java.concurrent;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.QueryBuilder;
import com.mongodb.WriteResult;
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
        DB db = MongodbTestBase.getDB( client );
        MongodbTestBase.dropCL( db, clName );
        DBCollection cl = db.getCollection( clName );
        cl.createIndex( new BasicDBObject( "a", 1 ), "a", false );
        List< DBObject > list = new ArrayList<>();
        for ( int i = 0; i < threadNumPerOpera * threadNumPerOpera; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b",
                    String.valueOf( i ) ) );
            list.add( new BasicDBObject( "c", i ).append( "d",
                    String.valueOf( i ) ) );
        }
        cl.insert( list );
    }

    @Test(enabled = false) // jira-6463
    public void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new CreateIndex( "a1" ) );
        threadExec.addWorker( new DeleteIndex( "a" ) );
        for ( int i = 0; i < threadNumPerOpera; i++ ) {
            // insert
            List< DBObject > list = new ArrayList<>();
            list.add( new BasicDBObject( "a1", threadNumPerOpera + i ) );
            list.add( new BasicDBObject( "b1",
                    String.valueOf( threadNumPerOpera + i ) ) );
            threadExec.addWorker( new Insert( list ) );

            // upsert [threadNumPerOpera*threadNumPerOpera,
            DBObject query = QueryBuilder.start( "a2" )
                    .is( i + threadNumPerOpera ).get();
            DBObject update = new BasicDBObject( "$set", new BasicDBObject(
                    "b2", String.valueOf( threadNumPerOpera + i ) ) );
            threadExec.addWorker( new Upsert( query, update ) );

            // update and delete
            // [0 threadNumPerOpera*threadNumPerOpera]
            DBObject query1 = QueryBuilder.start( "a" )
                    .greaterThan( i + threadNumPerOpera )
                    .lessThan( ( i + 1 ) * threadNumPerOpera ).get();

            threadExec.addWorker( new Delete( query1 ) );

            DBObject update1 = new BasicDBObject( "$set", new BasicDBObject(
                    "a", i + threadNumPerOpera * threadNumPerOpera ).append(
                            "e", i + threadNumPerOpera * threadNumPerOpera ) );
            threadExec.addWorker( new Update( query1, update1 ) );

            // count
            DBObject query2 = QueryBuilder.start( "c" )
                    .greaterThanEquals( i * threadNumPerOpera ).and( "c" )
                    .lessThan( ( i + 1 ) * threadNumPerOpera ).get();
            threadExec.addWorker( new Count( query2 ) );

            // find
            threadExec.addWorker( new Find( new BasicDBObject() ) );

            // aggregate
            List< DBObject > aggList = new ArrayList<>();
            aggList.add( new BasicDBObject( "$match", query2 ) );
            aggList.add( new BasicDBObject( "$group",
                    new BasicBSONObject( "_id", "$d" ).append( "sum_c",
                            new BasicBSONObject( "$sum", "$c" ) ) ) );
            aggList.add( new BasicDBObject( "$sort",
                    new BasicBSONObject( "_id", 1 ) ) );
            threadExec.addWorker( new Aggregate( aggList, new int[] {
                    i * threadNumPerOpera, ( i + 1 ) * threadNumPerOpera } ) );

            // distinct
            threadExec.addWorker( new Distinct( "c", query2, new int[] {
                    i * threadNumPerOpera, ( i + 1 ) * threadNumPerOpera } ) );
        }
        threadExec.run();

        // 更新的个数 = threadNumPerOpera * threadNumPerOpera - totalDelNum.get()
        DB db = MongodbTestBase.getDB( client );
        DBCollection cl = db.getCollection( clName );
        DBObject query = QueryBuilder.start( "e" ).exists( true ).and( "e" )
                .greaterThanEquals( -1 ).get();
        Assert.assertEquals( cl.count( query ), totalUpateNum.get() );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(),
                MongodbTestBase.getDB( client ), clName );
    }

    private class CreateIndex {
        private String keyAndName;

        public CreateIndex( String keyAndName ) {
            this.keyAndName = keyAndName;
        }

        @ExecuteOrder(step = 1)
        private void create() {
            DB db = MongodbTestBase.getDB( client );
            DBCollection cl = db.getCollection( clName );
            try {
                cl.createIndex( new BasicDBObject( keyAndName, 1 ), keyAndName,
                        false );
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "-48" ) ) {
                    throw e;
                }
            }
        }
    }

    private class Find {
        private DBObject query;

        public Find( DBObject query ) {
            this.query = query;
        }

        @ExecuteOrder(step = 1)
        private void create() {
            DB db = MongodbTestBase.getDB( client );
            DBCollection cl = db.getCollection( clName );
            try {
                cl.find( query );
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "-48" ) ) {
                    throw e;
                }
            }
        }
    }

    private class DeleteIndex {
        private String name;

        public DeleteIndex( String name ) {
            this.name = name;
        }

        @ExecuteOrder(step = 1)
        private void create() {
            DB db = MongodbTestBase.getDB( client );
            DBCollection cl = db.getCollection( clName );
            try {
                cl.dropIndex( name );
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "-48" ) ) {
                    throw e;
                }
            }
        }
    }

    private class Insert {
        private List< DBObject > documentList;

        public Insert( List< DBObject > documentList ) {
            this.documentList = documentList;
        }

        @ExecuteOrder(step = 1)
        private void insert() {
            DB db = MongodbTestBase.getDB( client );
            DBCollection cl = db.getCollection( clName );
            try {
                cl.insert( documentList );
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "-48" ) ) {
                    throw e;
                }
            }
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
            DB db = MongodbTestBase.getDB( client );
            DBCollection cl = db.getCollection( clName );
            try {
                cl.update( query, update, true, false );
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "-48" ) ) {
                    throw e;
                }
            }
        }
    }

    private class Update {
        private DBObject query;
        private DBObject update;

        public Update( DBObject query, DBObject update ) {
            this.query = query;
            this.update = update;
        }

        @ExecuteOrder(step = 1)
        private void update() {
            DB db = MongodbTestBase.getDB( client );
            DBCollection cl = db.getCollection( clName );
            try {
                WriteResult result = cl.update( query, update, false, true );
                totalUpateNum.getAndAdd( result.getN() );
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "-48" ) ) {
                    throw e;
                }
            }
        }
    }

    private class Delete {
        private DBObject query;

        public Delete( DBObject query ) {
            this.query = query;
        }

        @ExecuteOrder(step = 1)
        private void delete() {
            DB db = MongodbTestBase.getDB( client );
            DBCollection cl = db.getCollection( clName );
            try {
                WriteResult result = cl.remove( query );
                totalDelNum.getAndAdd( result.getN() );
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "-48" ) ) {
                    throw e;
                }
            }
        }
    }

    private class Count {
        private DBObject query;

        public Count( DBObject query ) {
            this.query = query;
        }

        @ExecuteOrder(step = 1)
        private void count() {
            DB db = MongodbTestBase.getDB( client );
            DBCollection cl = db.getCollection( clName );
            try {
                long count = cl.count( query );
                Assert.assertEquals( count, threadNumPerOpera );
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "-48" ) ) {
                    throw e;
                }
            }
        }
    }

    private class Distinct {
        private String filedName;
        private DBObject query;
        private int[] checkResult;

        public Distinct( String filedName, DBObject query, int[] checkResult ) {
            this.filedName = filedName;
            this.query = query;
            this.checkResult = checkResult;
        }

        @ExecuteOrder(step = 1)
        @SuppressWarnings("unchecked")
        private void distinct() {
            DB db = MongodbTestBase.getDB( client );
            DBCollection cl = db.getCollection( clName );
            try {
                List< Integer > result = ( List< Integer > ) cl
                        .distinct( filedName, query );
                List< Integer > checkList = new ArrayList<>();
                for ( int i = checkResult[ 0 ]; i < checkResult[ 1 ]; i++ ) {
                    checkList.add( i );
                }
                Assert.assertEquals( result, checkList );
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "-48" ) ) {
                    throw e;
                }
            }
        }
    }

    private class Aggregate {
        private List< DBObject > agg;
        private int[] checkResult;

        public Aggregate( List< DBObject > agg, int[] checkResult ) {
            this.agg = agg;
            this.checkResult = checkResult;
        }

        @ExecuteOrder(step = 1)
        private void aggregate() {
            DB db = MongodbTestBase.getDB( client );
            DBCollection cl = db.getCollection( clName );
            try {
                Iterator< DBObject > actResult = cl.aggregate( agg ).results()
                        .iterator();
                List< DBObject > checkList = new ArrayList<>();
                for ( int i = checkResult[ 0 ]; i < checkResult[ 1 ]; i++ ) {
                    checkList.add( new BasicDBObject( "_id", i + "" )
                            .append( "sum_c", ( double ) i ) );
                }
                int k = 0;
                while ( actResult.hasNext() ) {
                    Assert.assertEquals( actResult.next(), checkList.get( k ) );
                    k++;
                }
                Assert.assertEquals( k, checkList.size() );
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "-48" ) ) {
                    throw e;
                }
            }
        }
    }
}
