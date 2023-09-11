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
import com.mongodb.client.model.Aggregates;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33051:CRUD/aggregate操作，匹配条件为{a:null}
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class FiltersIsNull33051 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;

    @BeforeClass
    private void setUp() {
        clName = javaDBNameWithVersion + "_cl_33051";
        db = MongodbTestBase.getDataBase( client );
        System.out.println( db.getReadPreference() );
        cl = db.getCollection( clName );
        cl.drop();
        db.createCollection( clName );
        List< Document > docs = Arrays.asList( new Document( "_id", 1 ),
                new Document( "_id", 2 ).append( "a", null ),
                new Document( "_id", 3 ).append( "a", 3 ) );
        cl.insertMany( docs );
    }

    @Test
    private void test() {
        this.find_isNull();
        this.aggregate_isNull();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }

    private void find_isNull() {
        ArrayList< Document > actDocs;
        List< Document > expDocs = new ArrayList<>(
                Arrays.asList( new Document( "_id", 1 ),
                        new Document( "_id", 2 ).append( "a", null ) ) );

        // {a:null}
        actDocs = cl.find( new Document( "a", null ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actDocs, expDocs );

        // {a:{isnull:1}}
        actDocs.clear();
        actDocs = cl.find( new Document( "a", new Document( "$isnull", 1 ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actDocs, expDocs );

        // {a:{$ne:null}}
        actDocs.clear();
        expDocs.clear();
        actDocs = cl.find( new Document( "a", new Document( "$ne", null ) ) )
                .into( new ArrayList< Document >() );
        expDocs = new ArrayList<>(
                Arrays.asList( new Document( "_id", 3 ).append( "a", 3 ) ) );
        Assert.assertEquals( actDocs, expDocs );
    }

    private void aggregate_isNull() {
        Collection< Document > actDocs = cl
                .aggregate( Arrays.asList(
                        Aggregates.match( new Document( "a", null ) ) ) )
                .into( new ArrayList< Document >() );

        List< Document > expDocs = new ArrayList<>(
                Arrays.asList( new Document( "_id", 1 ),
                        new Document( "_id", 2 ).append( "a", null ) ) );

        Assert.assertEquals( actDocs, expDocs );
    }
}
