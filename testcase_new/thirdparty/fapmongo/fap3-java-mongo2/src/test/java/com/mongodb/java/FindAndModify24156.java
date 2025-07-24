package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-24156:findAndModify操作
 * @Author XiaoNi Huang
 * @Date 2021/4/29
 */
public class FindAndModify24156 extends MongodbTestBase {
    private DB mongoDB;
    private String clName;
    private DBCollection cl;

    @BeforeClass
    private void setUp() throws UnknownHostException {
        mongoDB = MongodbTestBase.getDB( client );
        clName = javaDBNameWithVersion + "_cl_24156";
        cl = mongoDB.getCollection( clName );
    }

    @Test
    private void test() {
        this.findAndModifyWithUpdate();
        this.findAndModifyWithRemove();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoDB, clName );
    }

    private void findAndModifyWithUpdate() {
        DBObject doc;
        List< DBObject > expDocs = new ArrayList<>();

        cl.remove( new BasicDBObject() );
        this.insertDocs();

        BasicDBObject query;
        BasicDBObject sort;
        BasicDBObject fields;
        BasicDBObject update;
        Boolean upsert;
        Boolean returnNew;
        Boolean remove;

        // update
        query = new BasicDBObject();
        update = new BasicDBObject( "$set", new BasicDBObject( "a", 2 ) );
        doc = cl.findAndModify( query, update );
        Assert.assertEquals( doc,
                new BasicDBObject( "_id", 1 ).append( "a", 1 ) );
        // check result
        expDocs.clear();
        expDocs.add( new BasicDBObject( "_id", 1 ).append( "a", 2 ) );
        expDocs.add( new BasicDBObject( "_id", 2 ).append( "a", 1 ) );
        expDocs.add( new BasicDBObject( "_id", 3 ).append( "a", 1 ) );
        this.checkResult( expDocs );

        // sort
        query = new BasicDBObject();
        sort = new BasicDBObject( "_id", -1 );
        update = new BasicDBObject( "$set", new BasicDBObject( "a", 3 ) );
        doc = cl.findAndModify( query, sort, update );
        Assert.assertEquals( doc,
                new BasicDBObject( "_id", 3 ).append( "a", 1 ) );
        // check result
        expDocs.clear();
        expDocs.add( new BasicDBObject( "_id", 1 ).append( "a", 2 ) );
        expDocs.add( new BasicDBObject( "_id", 2 ).append( "a", 1 ) );
        expDocs.add( new BasicDBObject( "_id", 3 ).append( "a", 3 ) );
        this.checkResult( expDocs );

        // query, fields, sort, remove, update, returnNew, upsert
        query = new BasicDBObject( "_id", new BasicDBObject( "$gt", 1 ) );
        fields = new BasicDBObject( "_id", 1 ).append( "c", 4 );
        sort = new BasicDBObject( "_id", -1 );
        remove = false;
        returnNew = true;
        upsert = true;
        update = new BasicDBObject( "$set",
                new BasicDBObject( "a", 4 ).append( "c", 4 ) );
        doc = cl.findAndModify( query, fields, sort, remove, update, returnNew,
                upsert );
        Assert.assertEquals( doc,
                new BasicDBObject( "_id", 3 ).append( "c", 4 ) );
        // check result
        expDocs.clear();
        expDocs.add( new BasicDBObject( "_id", 1 ).append( "a", 2 ) );
        expDocs.add( new BasicDBObject( "_id", 2 ).append( "a", 1 ) );
        expDocs.add( new BasicDBObject( "_id", 3 ).append( "a", 4 ).append( "c",
                4 ) );
        this.checkResult( expDocs );
    }

    private void findAndModifyWithRemove() {
        DBObject doc;
        List< DBObject > expDocs = new ArrayList<>();

        cl.remove( new BasicDBObject() );
        this.insertDocs();

        BasicDBObject query;
        BasicDBObject sort;
        BasicDBObject fields;
        BasicDBObject update;
        Boolean upsert;
        Boolean returnNew;
        Boolean remove;

        // query, fields, sort正序, remove, update, returnNew:false, upsert
        query = new BasicDBObject( "_id", new BasicDBObject( "$gt", 1 ) );
        fields = null;
        sort = new BasicDBObject( "_id", 1 );
        remove = true;
        returnNew = false;
        upsert = false;
        update = new BasicDBObject();
        doc = cl.findAndModify( query, fields, sort, remove, update, returnNew,
                upsert );
        Assert.assertEquals( doc,
                new BasicDBObject( "_id", 2 ).append( "a", 1 ) );
        // check result
        expDocs.clear();
        expDocs.add( new BasicDBObject( "_id", 1 ).append( "a", 1 ) );
        expDocs.add( new BasicDBObject( "_id", 3 ).append( "a", 1 ) );
        this.checkResult( expDocs );
    }

    private void insertDocs() {
        List< DBObject > insertDocs = new ArrayList<>();
        insertDocs.add( new BasicDBObject( "_id", 1 ).append( "a", 1 ) );
        insertDocs.add( new BasicDBObject( "_id", 2 ).append( "a", 1 ) );
        insertDocs.add( new BasicDBObject( "_id", 3 ).append( "a", 1 ) );
        cl.insert( insertDocs );
    }

    private void checkResult( List< DBObject > expDocs ) {
        List< DBObject > actDocs = cl.find()
                .sort( new BasicDBObject( "_id", 1 ) ).toArray();
        Assert.assertEquals( actDocs, expDocs, actDocs.toString() );
    }
}
