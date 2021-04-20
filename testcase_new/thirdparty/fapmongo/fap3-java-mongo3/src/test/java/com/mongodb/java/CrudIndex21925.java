package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.bson.Document;
import org.bson.conversions.Bson;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.MongoCommandException;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoCursor;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Filters;
import com.mongodb.client.model.IndexModel;
import com.mongodb.client.model.IndexOptions;
import com.mongodb.client.model.Indexes;
import com.mongodb.client.model.UpdateOptions;
import com.mongodb.client.model.Updates;
import com.mongodb.client.result.UpdateResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-21925:增删改查索引
 * @author fanyu
 * @Date:2020/3/12
 * @version:1.0
 */
public class CrudIndex21925 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private String[] indexNames = { "index21925A", "index21925B", "index21925C",
            "index21925D" };

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl21925";
    }

    @Test
    public void test() {
        db.createCollection( clName );
        MongoCollection< Document > cl = db.getCollection( clName );

        // 创建index
        cl.createIndex( Indexes.ascending( indexNames[ 0 ] ),
                new IndexOptions().unique( false ).name( indexNames[ 0 ] ) );
        cl.createIndex( Indexes.descending( indexNames[ 1 ] ) );
        cl.createIndexes( Collections.singletonList(
                new IndexModel( Indexes.descending( indexNames[ 2 ] ) ) ) );
        cl.createIndex( Indexes.ascending( "a" ),
                new IndexOptions().unique( true ).name( indexNames[ 3 ] ) );

        // 重复创建索引
        try {
            cl.createIndex( Indexes.ascending( indexNames[ 0 ] ),
                    new IndexOptions().unique( false )
                            .name( indexNames[ 0 ] ) );
            Assert.fail( "exp fail but act success" );
        } catch ( MongoCommandException e ) {
            if ( !e.getMessage().contains( "-247" ) ) {
                throw e;
            }
        }
        // 增删改查记录
        crud( cl );
        // 列取index
        MongoCursor< Document > cursor1 = cl.listIndexes().iterator();
        int i = 0;
        while ( cursor1.hasNext() ) {
            Document object = ( Document ) cursor1.next();
            if ( object.get( "name" ).toString().startsWith( "index21925" ) ) {
                Assert.assertEquals( object.get( "ns" ),
                        db.getName() + "." + clName );
            }
            i++;
        }
        Assert.assertEquals( i, indexNames.length + 1 );
        cursor1.close();

        // 删除index
        MongoCursor< Document > cursor2 = cl.listIndexes().iterator();
        while ( cursor2.hasNext() ) {
            Document object = ( Document ) cursor2.next();
            if ( object.getString( "name" ) != null
                    && object.getString( "name" ).startsWith( "index21925" ) ) {
                cl.dropIndex( object.getString( "name" ) );
            }
        }
        Assert.assertEquals(
                cl.listIndexes().into( new ArrayList< Document >() ).size(),
                1 );
        cursor2.close();
    }

    private void crud( MongoCollection< Document > cl ) {
        List< Document > list = new ArrayList<>();
        int num = 10;
        for ( int i = 0; i < num; i++ ) {
            list.add( new Document( "a", i ).append( "b", i + 1 )
                    .append( "c", i + 2 ).append( "d", i + 3 )
                    .append( "e", i + 4 ).append( "f", i + 5 )
                    .append( "g", i + 6 ) );
        }
        // 插入 无重复数据
        cl.insertMany( list );
        // 插入 有重复数据，索引键值重复
        for ( Document bson : list ) {
            bson.remove( "_id" );
        }
        try {
            cl.insertMany( list );
            Assert.fail( "exp fail but act success" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -38 ) {
                throw e;
            }
        }
        // 查询
        List< Document > result = ( List< Document > ) cl.find()
                .into( new ArrayList< Document >() );
        for ( int i = 0; i < result.size(); i++ ) {
            Document act = result.get( i );
            Document exp = list.get( i );
            act.remove( "_id" );
            exp.remove( "_id" );
            Assert.assertEquals( act.toString(), exp.toString() );
            i++;
        }

        Bson query;
        Bson update;
        UpdateResult actResult;
        // 更新 无重复数据
        query = Filters.eq( "a", 1 );
        update = Updates.set( "a", num + 1 );
        actResult = cl.updateMany( query, update );
        Assert.assertEquals( actResult.getMatchedCount(), 1 );
        Assert.assertEquals( actResult.getModifiedCount(), 1 );
        Assert.assertNull( actResult.getUpsertedId() );
        Assert.assertEquals( cl.count( new BasicDBObject( "a", num + 1 ) ), 1 );

        // 更新update 有重复数据:索引键值重复
        query = Filters.eq( "a", 2 );
        update = Updates.set( "a", num - 2 );
        try {
            cl.updateMany( query, update );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -38 ) {
                throw e;
            }
        }

        // 更新upsert 有重复数据:索引键值重复
        query = Filters.eq( "a", 2 );
        update = Updates.set( "a", num - 2 );
        try {
            cl.updateMany( query, update, new UpdateOptions().upsert( true ) );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -38 ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
