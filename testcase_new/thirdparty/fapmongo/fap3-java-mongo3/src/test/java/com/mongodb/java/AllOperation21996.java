package com.mongodb.java;

import static com.mongodb.client.model.Filters.and;
import static com.mongodb.client.model.Filters.eq;
import static com.mongodb.client.model.Filters.gte;
import static com.mongodb.client.model.Filters.lt;
import static com.mongodb.client.model.Filters.lte;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.mongodb.MongoClient;
import com.mongodb.MongoClientOptions;
import com.mongodb.ServerAddress;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoCursor;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Aggregates;
import com.mongodb.client.model.Sorts;
import com.mongodb.client.model.Updates;
import com.mongodb.client.result.UpdateResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21996:find/update/distinct/aggregate/count/delete大量数据
 * @author fanyu
 * @Date 2020/3/31
 * @version 1.00
 */
public class AllOperation21996 extends MongodbTestBase {
    private MongoDatabase db;
    private String clNameBase = "cl21996";
    private AtomicInteger clNum = new AtomicInteger( 5 );

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
    }

    @DataProvider(name = "data-provider", parallel = true)
    private Object[][] rangeData() {
        return new Object[][] {
                { clNameBase + String.valueOf( clNum.getAndDecrement() ), 999 },
                { clNameBase + String.valueOf( clNum.getAndDecrement() ),
                        1000 },
                { clNameBase + String.valueOf( clNum.getAndDecrement() ),
                        3001 },
                { clNameBase + String.valueOf( clNum.getAndDecrement() ),
                        5000 },
                { clNameBase + String.valueOf( clNum.getAndDecrement() ),
                        10000 } };
    }

    @SuppressWarnings("deprecation")
    @Test(dataProvider = "data-provider")
    public void test( String clName, int recordNum ) {
        MongoClient client1 = null;
        try {
            MongoClientOptions opt = MongoClientOptions.builder().build();
            client1 = new MongoClient(
                    new ServerAddress( config.getHost(), config.getPort() ),
                    opt );
            MongoDatabase db1 = client1.getDatabase( dbName );

            List< Document > list = new ArrayList<>();
            for ( int i = 0; i < recordNum; i++ ) {
                list.add( new Document( "a", i ).append( "b", i )
                        .append( "c", i ).append( "d", i )
                        .append( "e", "aaaaaaaaaaaaaaaaaaaaaaaaaaa" ) );
            }
            MongoCollection< Document > cl = db1.getCollection( clName );
            cl.insertMany( list );

            // find
            MongoCursor< Document > cursor1 = cl.find()
                    .sort( Sorts.ascending( "a" ) ).iterator();
            checkFindResult( cursor1, list );
            cursor1.close();

            // find limit 999
            MongoCursor< Document > cursor2 = cl.find().limit( 999 )
                    .sort( Sorts.ascending( "a" ) ).iterator();
            checkFindResult( cursor2,
                    list.subList( 0, Math.min( 999, recordNum ) ) );
            cursor2.close();

            // find limit 1000
            MongoCursor< Document > cursor3 = cl.find().limit( 1000 )
                    .sort( Sorts.ascending( "a" ) ).iterator();
            checkFindResult( cursor3,
                    list.subList( 0, Math.min( 1000, recordNum ) ) );
            cursor3.close();

            // find limit 1001
            MongoCursor< Document > cursor4 = cl.find().limit( 1001 )
                    .sort( Sorts.ascending( "a" ) ).iterator();
            checkFindResult( cursor4,
                    list.subList( 0, Math.min( 1001, recordNum ) ) );
            cursor4.close();

            // count
            Assert.assertEquals( cl.count(), recordNum );
            Assert.assertEquals(
                    cl.count( and( gte( "a", recordNum / 2 ),
                            lte( "a", recordNum ) ) ),
                    recordNum - recordNum / 2 );

            // distinct
            List< Object > list1 = cl.distinct( "a", Integer.class )
                    .into( new ArrayList<>() );
            Assert.assertEquals( list1.size(), recordNum );

            // update
            UpdateResult result = cl.updateMany( new Document(),
                    Updates.set( "b", recordNum ) );
            Assert.assertEquals( result.getMatchedCount(), recordNum );
            Assert.assertEquals( result.getModifiedCount(), recordNum );
            Assert.assertNull( result.getUpsertedId() );
            Assert.assertEquals( cl.count( eq( "b", recordNum ) ), recordNum );

            // aggregate
            Collection< Document > result1 = cl
                    .aggregate(
                            Arrays.asList(
                                    Aggregates.match( and( lt( "a", recordNum ),
                                            gte( "a", 0 ) ) ),
                                    Aggregates.sort( Sorts.ascending( "a" ) ) ),
                            Document.class )
                    .into( new ArrayList< Document >() );
            Assert.assertEquals( result1.size(), recordNum );
        } finally {
            if ( client1 != null )
                client1.close();

        }
    }

    private void checkFindResult( MongoCursor< Document > cursor,
            List< Document > expList ) {
        int i = 0;
        while ( cursor.hasNext() ) {
            Document actDocument = ( Document ) cursor.next();
            Document expDocument = expList.get( i );
            Assert.assertEquals( actDocument.get( "a" ), expDocument.get( "a" ),
                    "i = " + i + ",act = " + actDocument + "\n,exp = " + ""
                            + expDocument );
            Assert.assertEquals( actDocument.get( "b" ), expDocument.get( "b" ),
                    "i = " + i + ",act = " + actDocument + "\n,exp = " + ""
                            + expDocument );
            Assert.assertEquals( actDocument.get( "_id" ),
                    expDocument.get( "_id" ), "i = " + i + ",act = "
                            + actDocument + "\n,exp = " + "" + expDocument );
            i++;
        }
        Assert.assertEquals( i, expList.size() );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        String[] clNames = new String[] { clNameBase + 3, clNameBase + 2,
                clNameBase + 1 };
        dropCLByTestResult( context, this.toString(), db, clNames );
    }
}
