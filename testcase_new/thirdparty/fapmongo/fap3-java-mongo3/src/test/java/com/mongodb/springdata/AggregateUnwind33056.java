package com.mongodb.springdata;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.bson.Document;
import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.core.aggregation.Aggregation;
import org.springframework.data.mongodb.core.aggregation.AggregationResults;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33056:aggregate使用$unwind操作符
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class AggregateUnwind33056 extends MongodbTestBase {
    private String clName;

    @BeforeClass
    private void setUp() {
        clName = springDBNameWithVersion + "_cl_33056";
        mongoTemplate.dropCollection( clName );
        mongoTemplate.createCollection( clName );
        this.insertDocs();
    }

    @Test
    private void test() {
        Aggregation aggr = Aggregation.newAggregation(
                Aggregation.unwind( "$a", "arrayIndex", true ),
                Aggregation.group( "$a" ).avg( "$b" ).as( "avgB" ),
                Aggregation.sort( Sort.Direction.ASC, "avgB" ) );
        AggregationResults< BasicDBObject > aggrResults = mongoTemplate
                .aggregate( aggr, clName, BasicDBObject.class );

        List< BasicDBObject > expDocs = new ArrayList<>( Arrays.asList(
                new BasicDBObject( "_id", 1 ).append( "avgB", 1.0 ),
                new BasicDBObject( "_id", 2 ).append( "avgB", 1.5 ),
                new BasicDBObject( "_id", 3 ).append( "avgB", 2.5 ),
                new BasicDBObject( "a1", Arrays.asList( 1, 2 ) ).append( "avgB",
                        5.0 ),
                new BasicDBObject( "a1", Arrays.asList( 3, 4 ) ).append( "avgB",
                        5.0 ),
                new BasicDBObject( "_id", 6 ).append( "avgB", 6.0 ),
                new BasicDBObject( "_id", null ).append( "avgB",
                        6.333333333333333 ) ) );
        Assert.assertEquals( aggrResults, expDocs );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
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
        mongoTemplate.insert( docs, clName );
    }
}
