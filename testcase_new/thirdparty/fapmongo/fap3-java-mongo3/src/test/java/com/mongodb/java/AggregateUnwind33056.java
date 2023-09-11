package com.mongodb.java;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Accumulators;
import com.mongodb.client.model.Aggregates;
import com.mongodb.client.model.Sorts;
import com.mongodb.client.model.UnwindOptions;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33056:aggregate使用$unwind操作符
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class AggregateUnwind33056 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;

    @BeforeClass
    private void setUp() {
        clName = javaDBNameWithVersion + "_cl_33056";
        db = MongodbTestBase.getDataBase( client );
        cl = db.getCollection( clName );
        cl.drop();
        db.createCollection( clName );
        this.insertDocs();
    }

    @Test
    private void test() {
        Collection< Document > actDocs = cl.aggregate( Arrays.asList(
                Aggregates.unwind( "$a",
                        new UnwindOptions().preserveNullAndEmptyArrays( true )
                                .includeArrayIndex( "arrayIndex" ) ),
                Aggregates.group( "$a", Accumulators.avg( "avgB", "$b" ) ),
                Aggregates.sort( Sorts.ascending( "avgB" ) ) ) )
                .into( new ArrayList< Document >() );

        List< Document > expDocs = new ArrayList<>(
                Arrays.asList( new Document( "_id", 1 ).append( "avgB", 1.0 ),
                        new Document( "_id", 2 ).append( "avgB", 1.5 ),
                        new Document( "_id", 3 ).append( "avgB", 2.5 ),
                        new Document( "_id",
                                new Document( "a1", Arrays.asList( 1, 2 ) ) )
                                        .append( "avgB", 5.0 ),
                        new Document( "_id",
                                new Document( "a1", Arrays.asList( 3, 4 ) ) )
                                        .append( "avgB", 5.0 ),
                        new Document( "_id", 6 ).append( "avgB", 6.0 ),
                        new Document( "_id", null ).append( "avgB",
                                6.333333333333333 ) ) );

        Assert.assertEquals( actDocs, expDocs );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }

    private void insertDocs() {
        List< Document > docs = Arrays.asList(
                new Document( "_id", 1 ).append( "a", Arrays.asList( 1, 2 ) )
                        .append( "b", 1 ),
                new Document( "_id", 2 ).append( "a", Arrays.asList( 2, 3 ) )
                        .append( "b", 2 ),
                new Document( "_id", 3 ).append( "a", Arrays.asList( 3 ) )
                        .append( "b", 3 ),
                new Document( "_id", 4 ).append( "a", Arrays.asList() )
                        .append( "b", 4 ),
                new Document( "_id", 5 )
                        .append( "a", Arrays.asList(
                                new Document( "a1", Arrays.asList( 1, 2 ) ),
                                new Document( "a1", Arrays.asList( 3, 4 ) ) ) )
                        .append( "b", 5 ),
                new Document( "_id", 6 ).append( "a", 6 ).append( "b", 6 ),
                new Document( "_id", 7 ).append( "a", null ).append( "b", 7 ),
                new Document( "_id", 8 ).append( "b", 8 ) );
        cl.insertMany( docs );
    }
}
