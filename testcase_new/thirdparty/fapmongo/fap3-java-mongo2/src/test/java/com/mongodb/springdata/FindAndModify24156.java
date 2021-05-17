package com.mongodb.springdata;

import java.net.UnknownHostException;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.springframework.data.mongodb.core.FindAndModifyOptions;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.core.query.Update;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-24156:findAndModify操作
 * @Author XiaoNi Huang
 * @Date 2021/4/29
 */
public class FindAndModify24156 extends MongodbTestBase {
    private String clName;

    @BeforeClass
    private void setUp() throws UnknownHostException {
        clName = javaDBNameWithVersion + "_cl_24156";
    }

    @Test
    private void test() {
        this.findAndModifyWithUpdate();
        this.findAndModifyWithRemove();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
    }

    private void findAndModifyWithUpdate() {
        Entity doc;

        mongoTemplate.remove( new Query(), clName );
        this.insertDocs();

        Query query;
        Update update;
        FindAndModifyOptions fmOptions = new FindAndModifyOptions();

        // findAndModify( query, update, Class<T> )
        query = new Query();
        update = Update.update( "age", 2 );
        doc = mongoTemplate.findAndModify( query, update, Entity.class );
        Assert.assertEquals( doc, null );
        Assert.assertEquals(
                mongoTemplate.count(
                        new Query( Criteria.where( "age" ).is( 2 ) ), clName ),
                1 );

        // findAndModify( query, update, Class<T>, clName )
        query = new Query();
        update = Update.update( "age", 3 );
        doc = mongoTemplate.findAndModify( query, update, Entity.class,
                clName );
        Assert.assertEquals( doc.getAge(), 1 );
        Assert.assertEquals(
                mongoTemplate.count(
                        new Query( Criteria.where( "age" ).is( 3 ) ), clName ),
                2 );

        // findAndModify( query, update, FindAndModifyOptions, Class< T >,
        // clName )
        query = new Query( Criteria.where( "age" ).is( 4 ) );
        update = Update.update( "age", 4 );
        fmOptions.returnNew( true ).upsert( true );
        doc = mongoTemplate.findAndModify( query, update, fmOptions,
                Entity.class, clName );
        Assert.assertEquals( doc.getAge(), 4 );
        Assert.assertEquals(
                mongoTemplate.count(
                        new Query( Criteria.where( "age" ).is( 4 ) ), clName ),
                1 );
    }

    private void findAndModifyWithRemove() {
        Entity doc;

        mongoTemplate.remove( new Query(), clName );
        this.insertDocs();

        Query query;
        Update update;
        FindAndModifyOptions fmOptions = new FindAndModifyOptions();

        // findAndModify( query, update, remove, Class< T >,
        // clName )
        query = new Query( Criteria.where( "age" ).gt( 1 ) );
        update = Update.update( "age", 2 );
        fmOptions.returnNew( true ).remove( true );
        doc = mongoTemplate.findAndModify( query, update, fmOptions,
                Entity.class, clName );
        Assert.assertEquals( doc.getAge(), 2 );
        Assert.assertEquals(
                mongoTemplate.count(
                        new Query( Criteria.where( "age" ).is( 2 ) ), clName ),
                0 );
        Assert.assertEquals( mongoTemplate.count( new Query(), clName ), 2 );
    }

    private void insertDocs() {
        List< Entity > insertDocs = new CopyOnWriteArrayList<>();
        insertDocs.add( new Entity( "a1", "boy", 1, 1, Entity.COURSES ) );
        insertDocs.add( new Entity( "a2", "boy", 2, 2, Entity.COURSES ) );
        insertDocs.add( new Entity( "a3", "boy", 3, 3, Entity.COURSES ) );
        mongoTemplate.insert( insertDocs, clName );
    }
}
