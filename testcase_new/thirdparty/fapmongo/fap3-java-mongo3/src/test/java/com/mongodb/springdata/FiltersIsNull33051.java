package com.mongodb.springdata;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.bson.Document;
import org.springframework.data.mongodb.core.aggregation.Aggregation;
import org.springframework.data.mongodb.core.aggregation.AggregationResults;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33051:CRUD/aggregate操作，匹配条件为{a:null}
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class FiltersIsNull33051 extends MongodbTestBase {
    private String clName;

    @BeforeClass
    private void setUp() {
        clName = springDBNameWithVersion + "_cl_32902";
        mongoTemplate.dropCollection( clName );
        mongoTemplate.createCollection( clName );

        List< Document > docs = Arrays.asList( new Document( "_id", 1 ),
                new Document( "_id", 2 ).append( "a", null ),
                new Document( "_id", 3 ).append( "a", "3" ) );
        mongoTemplate.insert( docs, clName );
    }

    @Test
    private void test() {
        this.find_isNull();
        this.aggregate_isNull();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
    }

    private void find_isNull() {
        Query query;
        List< MyEntity > actDocs;

        // {a:null}
        query = new Query( Criteria.where( "a" ).is( null ) );
        Assert.assertEquals( query.getQueryObject().toString(),
                "{ \"a\" :  null }" );
        actDocs = mongoTemplate.find( query, MyEntity.class, clName );
        Assert.assertEquals( actDocs.size(), 2 );
        for ( MyEntity doc : actDocs )
            Assert.assertNull( doc.getA() );
        actDocs.clear();

        // {a:{$isnull:1}}
        query = new Query(
                Criteria.where( "a" ).is( new Document( "$isnull", 1 ) ) );
        Assert.assertEquals( query.getQueryObject().toString(),
                "{ \"a\" : { \"$isnull\" : 1}}" );
        actDocs = mongoTemplate.find( query, MyEntity.class, clName );
        Assert.assertEquals( actDocs.size(), 2 );
        for ( MyEntity doc : actDocs )
            Assert.assertNull( doc.getA() );
        actDocs.clear();

        // {a:{$ne:null}}
        query = new Query( Criteria.where( "a" ).ne( null ) );
        actDocs = mongoTemplate.find( query, MyEntity.class, clName );
        Assert.assertEquals( actDocs.size(), 1 );
        for ( MyEntity doc : actDocs )
            Assert.assertNotNull( doc.getA() );
        actDocs.clear();
    }

    private void aggregate_isNull() {
        Aggregation aggr = Aggregation.newAggregation(
                Aggregation.match( Criteria.where( "a" ).is( null ) ),
                Aggregation.project( "a" ) );
        AggregationResults< BasicDBObject > aggrResults = mongoTemplate
                .aggregate( aggr, clName, BasicDBObject.class );
        List< BasicDBObject > expDocs = new ArrayList<>( Arrays.asList(
                new BasicDBObject( "_id", 1 ).append( "a", null ),
                new BasicDBObject( "_id", 2 ).append( "a", null ) ) );
        Assert.assertEquals( aggrResults, expDocs );
    }

    private class MyEntity implements Serializable {
        private static final long serialVersionUID = 1L;
        private int id;
        private String a;

        private MyEntity( int id, String a ) {
            super();
            this.id = id;
            this.a = a;
        }

        @SuppressWarnings("unused")
        private int getId() {
            return id;
        }

        private String getA() {
            return a;
        }
    }
}
