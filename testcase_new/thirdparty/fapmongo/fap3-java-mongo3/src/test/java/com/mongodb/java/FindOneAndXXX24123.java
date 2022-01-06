package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.FindIterable;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.FindOneAndDeleteOptions;
import com.mongodb.client.model.FindOneAndReplaceOptions;
import com.mongodb.client.model.FindOneAndUpdateOptions;
import com.mongodb.client.model.ReturnDocument;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-24123:findAndModify操作
 * @Author XiaoNi Huang
 * @Date 2021/4/29
 */
public class FindOneAndXXX24123 extends MongodbTestBase {
    private MongoDatabase mongoDB;
    private String clName;
    private MongoCollection< Document > cl;

    @BeforeClass
    private void setUp() throws UnknownHostException {
        mongoDB = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl_24123";
        cl = mongoDB.getCollection( clName );
    }

    @Test
    private void test() {
        this.findOneAndUpdate();
        this.findOneAndReplace();
        this.findOneAndDelete();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoDB, clName );
    }

    private void findOneAndUpdate() {
        Document doc;
        List< Document > expDocs = new ArrayList<>();

        cl.deleteMany( new Document() );
        this.insertDocs();

        Document query;
        Document update;
        FindOneAndUpdateOptions updateOptions;

        // findOneAndUpdate
        query = new Document();
        update = new Document( "$set", new Document( "a", 2 ) );
        doc = cl.findOneAndUpdate( query, update );
        Assert.assertEquals( doc, new Document( "_id", 1 ).append( "a", 1 ) );
        // check result
        expDocs.clear();
        expDocs.add( new Document( "_id", 1 ).append( "a", 2 ) );
        expDocs.add( new Document( "_id", 2 ).append( "a", 1 ) );
        expDocs.add( new Document( "_id", 3 ).append( "a", 1 ) );
        this.checkResult( expDocs );

        // ReturnDocument.BEFORE
        query = new Document();
        update = new Document( "$set", new Document( "a", 3 ) );
        updateOptions = new FindOneAndUpdateOptions()
                .returnDocument( ReturnDocument.BEFORE );
        doc = cl.findOneAndUpdate( query, update, updateOptions );
        Assert.assertEquals( doc, new Document( "_id", 1 ).append( "a", 2 ) );
        // check result
        expDocs.clear();
        expDocs.add( new Document( "_id", 1 ).append( "a", 3 ) );
        expDocs.add( new Document( "_id", 2 ).append( "a", 1 ) );
        expDocs.add( new Document( "_id", 3 ).append( "a", 1 ) );
        this.checkResult( expDocs );

        // sort, upsert, ReturnDocument.AFTER
        query = new Document( "_id", new Document( "$gt", 1 ) );
        update = new Document( "$set", new Document( "a", 4 ) );
        updateOptions = new FindOneAndUpdateOptions()
                .sort( new Document( "_id", -1 ) ).upsert( true )
                .returnDocument( ReturnDocument.AFTER );
        doc = cl.findOneAndUpdate( query, update, updateOptions );
        Assert.assertEquals( doc, new Document( "_id", 3 ).append( "a", 4 ) );
        // check result
        expDocs.clear();
        expDocs.add( new Document( "_id", 1 ).append( "a", 3 ) );
        expDocs.add( new Document( "_id", 2 ).append( "a", 1 ) );
        expDocs.add( new Document( "_id", 3 ).append( "a", 4 ) );
        this.checkResult( expDocs );
    }

    private void findOneAndReplace() {
        Document doc;
        List< Document > expDocs = new ArrayList<>();

        cl.deleteMany( new Document() );
        this.insertDocs();

        Document query;
        Document replace;
        FindOneAndReplaceOptions replaceOptions;

        // findOneAndReplace
        query = new Document();
        replace = new Document( "a", 2 );
        doc = cl.findOneAndReplace( query, replace );
        Assert.assertEquals( doc, new Document( "_id", 1 ).append( "a", 1 ) );
        // check result
        expDocs.clear();
        expDocs.add( new Document( "_id", 1 ).append( "a", 2 ) );
        expDocs.add( new Document( "_id", 2 ).append( "a", 1 ) );
        expDocs.add( new Document( "_id", 3 ).append( "a", 1 ) );
        this.checkResult( expDocs );

        // ReturnDocument.BEFORE
        query = new Document();
        replace = new Document( "b", 3 );
        replaceOptions = new FindOneAndReplaceOptions()
                .returnDocument( ReturnDocument.BEFORE );
        doc = cl.findOneAndReplace( query, replace, replaceOptions );
        Assert.assertEquals( doc, new Document( "_id", 1 ).append( "a", 2 ) );
        // check result
        expDocs.clear();
        expDocs.add( new Document( "_id", 1 ).append( "b", 3 ) );
        expDocs.add( new Document( "_id", 2 ).append( "a", 1 ) );
        expDocs.add( new Document( "_id", 3 ).append( "a", 1 ) );
        this.checkResult( expDocs );

        // sort, upsert, ReturnDocument.AFTER
        query = new Document( "_id", new Document( "$gt", 1 ) );
        replace = new Document( "c", 4 );
        replaceOptions = new FindOneAndReplaceOptions()
                .sort( new Document( "_id", -1 ) ).upsert( true )
                .returnDocument( ReturnDocument.AFTER );
        doc = cl.findOneAndReplace( query, replace, replaceOptions );
        Assert.assertEquals( doc, new Document( "_id", 3 ).append( "c", 4 ) );
        // check result
        expDocs.clear();
        expDocs.add( new Document( "_id", 1 ).append( "b", 3 ) );
        expDocs.add( new Document( "_id", 2 ).append( "a", 1 ) );
        expDocs.add( new Document( "_id", 3 ).append( "c", 4 ) );
        this.checkResult( expDocs );
    }

    private void findOneAndDelete() {
        Document doc;
        List< Document > expDocs = new ArrayList<>();

        cl.deleteMany( new Document() );
        this.insertDocs();

        Document query;
        FindOneAndDeleteOptions deleteOptions;

        // findOneAndDelete
        query = new Document();
        doc = cl.findOneAndDelete( query );
        Assert.assertEquals( doc, new Document( "_id", 1 ).append( "a", 1 ) );
        // check result
        expDocs.clear();
        expDocs.add( new Document( "_id", 2 ).append( "a", 1 ) );
        expDocs.add( new Document( "_id", 3 ).append( "a", 1 ) );
        this.checkResult( expDocs );

        // sort
        query = new Document( "_id", new Document( "$gt", 1 ) );
        deleteOptions = new FindOneAndDeleteOptions()
                .sort( new Document( "_id", -1 ) );
        doc = cl.findOneAndDelete( query, deleteOptions );
        Assert.assertEquals( doc, new Document( "_id", 3 ).append( "a", 1 ) );
        // check result
        expDocs.clear();
        expDocs.add( new Document( "_id", 2 ).append( "a", 1 ) );
        this.checkResult( expDocs );
    }

    private void insertDocs() {
        List< Document > insertDocs = new ArrayList<>();
        insertDocs.add( new Document( "_id", 1 ).append( "a", 1 ) );
        insertDocs.add( new Document( "_id", 2 ).append( "a", 1 ) );
        insertDocs.add( new Document( "_id", 3 ).append( "a", 1 ) );
        cl.insertMany( insertDocs );
    }

    private void checkResult( List< Document > expDocs ) {
        FindIterable< Document > fi = cl.find()
                .sort( new Document( "_id", 1 ) );
        List< Document > actDocs = fi.into( new ArrayList< Document >() );
        Collections.sort( actDocs, new Comparator< Document >() {
            @Override
            public int compare( Document o1, Document o2 ) {
                return o1.getInteger( "_id" ) - o2.getInteger( "_id" );
            }
        } );
        Assert.assertEquals( actDocs, expDocs, actDocs.toString() );
    }
}
