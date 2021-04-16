package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.MongoClient;
import com.mongodb.MongoClientOptions;
import com.mongodb.QueryBuilder;
import com.mongodb.ServerAddress;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21996:find/update/distinct/aggregate/count/delete大量数据
 * @author fanyu
 * @Date 2020/3/31
 * @version 1.00
 */
public class AllOperation21996 extends MongodbTestBase {
    private DB db;
    private String clNameBase = "cl21996c_";
    private AtomicInteger clNum = new AtomicInteger( 3 );

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
    }

    @DataProvider(name = "data-provider", parallel = true)
    private Object[][] rangeData() {
        return new Object[][] {
                { clNameBase + String.valueOf( clNum.getAndDecrement() ), 999 },
                { clNameBase + String.valueOf( clNum.getAndDecrement() ),
                        1000 },
                { clNameBase + String.valueOf( clNum.getAndDecrement() ),
                        10000 } };
    }

    @SuppressWarnings("unchecked")
    @Test(dataProvider = "data-provider")
    public void test( String clName, int recordNum )
            throws UnknownHostException {
        MongoClient client1 = null;
        try {
            MongoClientOptions opt = MongoClientOptions.builder().build();
            client1 = new MongoClient(
                    new ServerAddress( config.getHost(), config.getPort() ),
                    opt );
            DB db1 = client1.getDB( dbName );

            List< DBObject > list = new ArrayList<>();
            for ( int i = 0; i < recordNum; i++ ) {
                list.add( new BasicDBObject( "a", i ).append( "b", i )
                        .append( "c", i ).append( "d", i )
                        .append( "e", "aaaaaaaaaaaaaaaaaaaaaaaaaaa" ) );
            }
            DBCollection cl = db1.getCollection( clName );
            cl.insert( list );

            // find
            List< DBObject > result1 = cl.find()
                    .sort( new BasicDBObject( "a", 1 ) ).toArray();
            Assert.assertEquals( result1, list );

            // find limit 999
            List< DBObject > result2 = cl.find().limit( 999 )
                    .sort( new BasicDBObject( "a", 1 ) ).toArray();
            Assert.assertEquals( result2,
                    list.subList( 0, Math.min( 999, recordNum ) ) );

            // find limit 1000
            List< DBObject > result3 = cl.find().limit( 1000 )
                    .sort( new BasicDBObject( "a", 1 ) ).toArray();
            Assert.assertEquals( result3,
                    list.subList( 0, Math.min( 1000, recordNum ) ) );

            // find limit 1001
            List< DBObject > result4 = cl.find().limit( 1001 )
                    .sort( new BasicDBObject( "a", 1 ) ).toArray();
            Assert.assertEquals( result4,
                    list.subList( 0, Math.min( 1001, recordNum ) ) );

            // count
            Assert.assertEquals( cl.count(), recordNum );

            // distinct
            List< Object > result5 = cl.distinct( "a" );
            Assert.assertEquals( result5.size(), recordNum );

            // update
            DBObject query2 = QueryBuilder.start( "b" ).greaterThanEquals( 0 )
                    .get();
            DBObject update2 = new BasicDBObject( "$set",
                    new BasicDBObject( "b", recordNum ) );
            cl.updateMulti( query2, update2 );
            Assert.assertEquals(
                    cl.count( new BasicDBObject( "b", recordNum ) ),
                    recordNum );

            // aggregate
            // 匹配记录数超过1000，聚集操作返回的结果不正确，
            // 与开发确认，暂时不在2.14.2版本上解决
            // List< DBObject > list2 = new ArrayList<>();
            // list2.add( new BasicDBObject( "$match", QueryBuilder.start( "a" )
            // .greaterThanEquals( 0 ).lessThan( recordNum ).get() ) );;
            // Iterator< DBObject > result6 = cl.aggregate( list2 ).results()
            // .iterator();
            // int k1 = 0;
            // while ( result6.hasNext() ) {
            // System.out.println( result6.next().toString());
            // k1++;
            // }
            // Assert.assertEquals( k1, recordNum );
        } finally {
            if ( client1 != null )
                client1.close();
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        String[] clNames = new String[] { clNameBase + 3, clNameBase + 2,
                clNameBase + 1 };
        dropCLByTestResult( context, this.toString(), db, clNames );
    }
}
