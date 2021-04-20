package com.mongodb.java;

import static com.mongodb.client.model.Filters.eq;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.bson.Document;
import org.bson.conversions.Bson;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.MongoCommandException;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoCursor;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.CreateCollectionOptions;
import com.mongodb.client.model.Filters;
import com.mongodb.client.model.Indexes;
import com.mongodb.client.model.UpdateOptions;
import com.mongodb.client.model.Updates;
import com.mongodb.client.model.ValidationOptions;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21924:增删改查集合
 * @author fanyu
 * @Date 2020/3/22
 * @version 1.00
 */
public class CrudCL21924 extends MongodbTestBase {
    private MongoDatabase db;
    private String[] clNames = new String[ 2 ];

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clNames[ 0 ] = javaDBNameWithVersion + "_cl21924A";
        clNames[ 1 ] = javaDBNameWithVersion + "_cl21924B";
    }

    @Test
    public void test() {
        // 不带条件创建cl
        db.createCollection( clNames[ 0 ] );

        // 带条件创建cl
        ValidationOptions collOptions = new ValidationOptions()
                .validator( Filters.or( Filters.exists( "email" ),
                        Filters.exists( "phone" ) ) );
        db.createCollection( clNames[ 1 ], new CreateCollectionOptions()
                .validationOptions( collOptions ) );
        // 重复创建集合
        try {
            db.createCollection( clNames[ 1 ], new CreateCollectionOptions()
                    .validationOptions( collOptions ) );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -22 ) {
                throw e;
            }
        }
        // 列取cl
        MongoCursor< String > cursor = db.listCollectionNames().iterator();
        List< String > actClNames = new ArrayList<>();
        while ( cursor.hasNext() ) {
            actClNames.add( cursor.next() );
        }
        Collections.sort( actClNames );
        Assert.assertTrue( actClNames.containsAll( Arrays.asList( clNames ) ) );

        // 删除cl
        for ( int i = 0; i < clNames.length; i++ ) {
            db.getCollection( clNames[ i ] ).drop();
        }
        Assert.assertFalse(
                db.listCollectionNames().into( new ArrayList< String >() )
                        .containsAll( Arrays.asList( clNames ) ) );
    }

    @Test
    public void test2() {
        String dbName = javaDBNameWithVersion + "_db21924";
        String clName = javaDBNameWithVersion + "_cl21924_test2";
        MongoDatabase db = client.getDatabase( dbName );
        MongoCollection< Document > cl = db.getCollection( clName );
        db.drop();
        // cs和cl都不存在，执行insert、upsert、createIndex、createCL 操作,创建cs和cl
        autoCreate( db, cl, clName );
        // cs存在，cl不存在，执行insert、upsert、createIndex、createCL操作
        db.createCollection( clName );
        cl.drop();
        autoCreate( db, cl, clName );
    }

    private void autoCreate( MongoDatabase db, MongoCollection< Document > cl,
            String clName ) {
        // insert
        cl.insertOne( new Document( "a", 1 ) );
        Assert.assertTrue( db.listCollectionNames()
                .into( new ArrayList< String >() ).contains( clName ) );
        db.drop();

        // upsert
        Assert.assertFalse( db.listCollectionNames()
                .into( new ArrayList< String >() ).contains( clName ) );
        Bson query = Filters.and( eq( "a", 0 ) );
        Bson update = Updates.set( "a", 1 );
        cl.updateMany( query, update, new UpdateOptions().upsert( true ) );
        Assert.assertTrue( db.listCollectionNames()
                .into( new ArrayList< String >() ).contains( clName ) );
        db.drop();

        // createIndex
        Assert.assertFalse( db.listCollectionNames()
                .into( new ArrayList< String >() ).contains( clName ) );
        cl.createIndex( Indexes.descending( "a" ) );
        Assert.assertTrue( db.listCollectionNames()
                .into( new ArrayList< String >() ).contains( clName ) );
        db.drop();

        // create cl
        Assert.assertFalse( db.listCollectionNames()
                .into( new ArrayList< String >() ).contains( clName ) );
        db.createCollection( clName );
        Assert.assertTrue( db.listCollectionNames()
                .into( new ArrayList< String >() ).contains( clName ) );
        db.drop();
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
    }
}
