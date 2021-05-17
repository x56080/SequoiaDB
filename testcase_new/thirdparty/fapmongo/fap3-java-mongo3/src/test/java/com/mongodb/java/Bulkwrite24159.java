package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.bson.Document;
import org.bson.conversions.Bson;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.FindIterable;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.BulkWriteOptions;
import com.mongodb.client.model.DeleteOneModel;
import com.mongodb.client.model.Filters;
import com.mongodb.client.model.InsertOneModel;
import com.mongodb.client.model.ReplaceOneModel;
import com.mongodb.client.model.ReplaceOptions;
import com.mongodb.client.model.UpdateOneModel;
import com.mongodb.client.model.UpdateOptions;
import com.mongodb.client.model.WriteModel;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-24159:bulkwrite操作
 * @Author XiaoNi Huang
 * @Date 2021/4/29
 */
public class Bulkwrite24159 extends MongodbTestBase {
    private MongoDatabase mongoDB;
    private String clName;
    private MongoCollection< Document > cl;

    @BeforeClass
    private void setUp() throws UnknownHostException {
        mongoDB = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl_24159";
        cl = mongoDB.getCollection( clName );
    }

    @Test
    private void test() {
        this.test1_bulkwrite();
        this.test2_bulkwrite();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoDB, clName );
    }

    private void test1_bulkwrite() {
        // test: cl.bulkWrite( arg0 )
        cl.deleteMany( new Document() );
        List< WriteModel< Document > > writeMode = new ArrayList<>();

        // InsertOneModel
        for ( int i = 1; i < 5; i++ ) {
            Document insertDoc = new Document( "_id", i ).append( "a", i );
            InsertOneModel< Document > insertOneMode = new InsertOneModel< Document >(
                    insertDoc );
            writeMode.add( insertOneMode );
        }

        // UpdateOneMode
        Bson updateFilters = Filters.gt( "_id", 0 );
        Document updateDoc = new Document( "$set", new Document( "b", 1 ) );
        UpdateOneModel< Document > updateOneMode = new UpdateOneModel< Document >(
                updateFilters, updateDoc );

        writeMode.add( updateOneMode );

        // ReplaceOneMode
        Bson replaceFilters = Filters.gt( "_id", 1 );
        Document replaceDoc = new Document( "c", 1 );
        ReplaceOneModel< Document > replaceOneMode = new ReplaceOneModel< Document >(
                replaceFilters, replaceDoc );

        writeMode.add( replaceOneMode );

        // DeleteOneMode
        Bson deleteFilters = Filters.gt( "_id", 2 );
        DeleteOneModel< Document > deleteOneMode = new DeleteOneModel< Document >(
                deleteFilters );

        writeMode.add( deleteOneMode );

        // BulkWrite
        cl.bulkWrite( writeMode );
        // check result
        List< Document > expDocs = new ArrayList<>();
        expDocs.add(
                new Document( "_id", 1 ).append( "a", 1 ).append( "b", 1 ) );
        expDocs.add( new Document( "_id", 2 ).append( "c", 1 ) );
        expDocs.add( new Document( "_id", 4 ).append( "a", 4 ) );
        this.checkResult( expDocs );
    }

    private void test2_bulkwrite() {
        // test: cl.bulkWrite( arg0, arg1 )
        cl.deleteMany( new Document() );
        List< WriteModel< Document > > writeMode = new ArrayList<>();

        // UpdateOneMode, updateOptions
        Bson updateFilters = Filters.eq( "_id", 1 );
        Document updateDoc = new Document( "$set",
                new Document( "_id", 1 ).append( "a", 1 ) );
        UpdateOptions updateOptions = new UpdateOptions().upsert( true );
        UpdateOneModel< Document > updateOneMode = new UpdateOneModel< Document >(
                updateFilters, updateDoc, updateOptions );

        writeMode.add( updateOneMode );

        // ReplaceOneMode
        Bson replaceFilters = Filters.eq( "_id", 2 );
        Document replaceDoc = new Document( "_id", 2 ).append( "a", 2 );
        ReplaceOptions replaceOptions = new ReplaceOptions().upsert( true );
        ReplaceOneModel< Document > replaceOneMode = new ReplaceOneModel< Document >(
                replaceFilters, replaceDoc, replaceOptions );

        writeMode.add( replaceOneMode );

        // BulkWrite
        BulkWriteOptions bwOptions = new BulkWriteOptions();
        bwOptions.ordered( true );
        cl.bulkWrite( writeMode );
        // check result
        List< Document > expDocs = new ArrayList<>();
        expDocs.add( new Document( "_id", 1 ).append( "a", 1 ) );
        expDocs.add( new Document( "_id", 2 ).append( "a", 2 ) );
        this.checkResult( expDocs );
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
