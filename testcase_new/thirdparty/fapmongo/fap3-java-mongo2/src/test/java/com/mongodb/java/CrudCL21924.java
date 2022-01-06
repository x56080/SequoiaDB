package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBCursor;
import com.mongodb.DBObject;
import com.mongodb.MongoCommandException;
import com.mongodb.WriteResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21924:增删改查集合
 * @author fanyu
 * @Date 2020/3/12
 * @version 1.00
 */
public class CrudCL21924 extends MongodbTestBase {
    private DB db;
    private String[] clNames = new String[ 2 ];

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
        clNames[ 0 ] = javaDBNameWithVersion + "_cl21924A";
        clNames[ 1 ] = javaDBNameWithVersion + "_cl21924B";
    }

    @Test
    public void test() {
        // 不带条件，集合不存在，创建集合
        db.createCollection( clNames[ 0 ], new BasicDBObject() );
        // 集合存在，创建集合
        try {
            db.createCollection( clNames[ 0 ], new BasicDBObject() );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -22 ) {
                throw e;
            }
        }
        // 带条件创建集合，sequoiadb会忽略这些条件
        db.createCollection( clNames[ 1 ], new BasicDBObject( "test", 1 ) );

        // 判断集合存不存在
        for ( String clName : clNames ) {
            Assert.assertTrue( db.collectionExists( clName ) );
        }
        Assert.assertFalse(
                db.collectionExists( clNames[ 0 ] + "_inexitences" ) );
        // 列取集合名
        DB db1 = client.getDB( "db21924" );
        db1.createCollection( clNames[ 0 ], new BasicDBObject() );
        db1.getCollection( clNames[ 0 ] ).drop();
        // 存在集合的db列取集合
        Set< String > actClNames = db.getCollectionNames();
        Assert.assertTrue( actClNames.size() >= clNames.length );
        Assert.assertTrue( actClNames.containsAll( Arrays.asList( clNames ) ) );
        // 不存在集合的db列取集合
        Assert.assertEquals( db1.getCollectionNames().size(), 0 );
        db1.dropDatabase();

        // 获取集合
        DBCollection cl1 = db.getCollection( clNames[ 0 ] );
        DBCollection cl2 = db.getCollectionFromString( clNames[ 1 ] );

        // 做简单的增删改查操作
        crud( cl1 );
        crud( cl2 );

        // 刪除集合
        cl1.drop();
        cl2.drop();
        for ( String clName : clNames ) {
            Assert.assertFalse( db.collectionExists( clName ) );
        }
        // TODO: 执行成功，不会报错
        // try {
        // db1.getCollection( clNames[ 0 ] + "_inexistence" ).drop();
        // Assert.fail( "exp fail but act success" );
        // } catch ( MongoCommandException e ) {
        // if ( e.getErrorCode() != -23 ) {
        // throw e;
        // }
        // }
    }

    @Test
    public void test2() {
        String dbName = javaDBNameWithVersion + "_db21924";
        String clName = javaDBNameWithVersion + "_cl21924_test2";
        DB db = client.getDB( dbName );
        DBCollection cl = db.getCollection( clName );
        db.dropDatabase();
        // cs和cl都不存在，执行insert、upsert、createIndex、createCL 操作,创建cs和cl
        autoCreate( db, cl, clName );
        // cs存在，cl不存在，执行insert、upsert、createIndex、createCL操作
        cl = db.createCollection( clName, new BasicDBObject() );
        cl.drop();
        autoCreate( db, cl, clName );
    }

    private void autoCreate( DB db, DBCollection cl, String clName ) {
        // insert
        cl.insert( new BasicDBObject( "a", 1 ) );
        db.collectionExists( clName );
        Assert.assertTrue( db.collectionExists( clName ) );
        db.dropDatabase();

        // upsert
        Assert.assertFalse( db.collectionExists( clName ) );
        cl.update( new BasicDBObject( "a", 1 ), new BasicDBObject( "a", 2 ),
                true, false );
        Assert.assertTrue( db.collectionExists( clName ) );
        db.dropDatabase();

        // createIndex
        Assert.assertFalse( db.collectionExists( clName ) );
        cl.createIndex( "b" );
        Assert.assertTrue( db.collectionExists( clName ) );
        db.dropDatabase();

        // create cl
        Assert.assertFalse( db.collectionExists( clName ) );
        db.createCollection( clName, new BasicDBObject() );
        Assert.assertTrue( db.collectionExists( clName ) );
        db.dropDatabase();
    }

    private void crud( DBCollection cl ) {
        List< DBObject > list = new ArrayList<>();
        int num = 10;
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b", i + 1 ) );
        }
        // 插入
        cl.insert( list );
        // 查询
        DBCursor result2 = cl.find().sort( new BasicDBObject( "a", 1 ) );
        int i = 0;
        while ( result2.hasNext() ) {
            DBObject act = result2.next();
            DBObject exp = list.get( i );
            Assert.assertEquals( act.removeField( "_id" ),
                    exp.removeField( "_id" ) );
            Assert.assertEquals( act.toString(), exp.toString() );
            i++;
        }
        Assert.assertEquals( i, num );

        // 更新
        WriteResult result3 = cl.update( new BasicDBObject( "a", 1 ),
                new BasicDBObject( "$set",
                        new BasicBSONObject( "a", num + 1 ) ) );
        Assert.assertEquals( result3.getN(), 1 );
        Assert.assertNull( result3.getUpsertedId() );
        Assert.assertEquals( cl.count( new BasicDBObject( "a", num + 1 ) ), 1 );

        // 刪除
        WriteResult result4 = cl.remove( new BasicDBObject() );
        Assert.assertEquals( result4.getN(), num );
        Assert.assertNull( result4.getUpsertedId() );
        Assert.assertEquals( cl.count( new BasicDBObject() ), 0 );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
    }
}
