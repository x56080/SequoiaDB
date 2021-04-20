package com.mongodb.springdata;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

import org.bson.BasicBSONObject;
import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.UncategorizedMongoDbException;
import org.springframework.data.mongodb.core.CollectionOptions;
import org.springframework.data.mongodb.core.MongoTemplate;
import org.springframework.data.mongodb.core.index.Index;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.core.query.Update;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DBCollection;
import com.mongodb.DBCursor;
import com.mongodb.DBObject;
import com.mongodb.WriteResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21924:增删改查集合
 * @author fanyu
 * @Date 2020/3/19
 * @version 1.00
 */
public class CrudCL21924 extends MongodbTestBase {
    private String[] clNames = new String[ 2 ];

    @BeforeClass
    public void setUp() {
        clNames[ 0 ] = springDBNameWithVersion + "_cl21924A";
        clNames[ 1 ] = springDBNameWithVersion + "_cl21924B";
    }

    @Test
    public void test() {
        // 不带条件，集合不存在，创建集合
        mongoTemplate.createCollection( clNames[ 0 ] );
        // spring 执行命令会卡住
        // 集合存在，创建集合
        try {
            mongoTemplate.createCollection( clNames[ 0 ] );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "-22" ) ) {
                throw e;
            }
        }
        // 带条件创建集合，sequoiadb会忽略这些条件
        mongoTemplate.createCollection( clNames[ 1 ],
                new CollectionOptions( 3, 10, false ) );

        // 判断集合存不存在
        for ( String clName : clNames ) {
            Assert.assertTrue( mongoTemplate.collectionExists( clName ) );
        }
        Assert.assertFalse( mongoTemplate
                .collectionExists( clNames[ 0 ] + "_inexitences" ) );

        // 存在集合的db列取集合,不考虑不存在集合的db
        Set< String > actClNames = mongoTemplate.getCollectionNames();
        Assert.assertTrue( actClNames.size() >= clNames.length );
        Assert.assertTrue( actClNames.containsAll( Arrays.asList( clNames ) ) );

        // 获取集合
        DBCollection cl1 = mongoTemplate.getCollection( clNames[ 0 ] );
        DBCollection cl2 = mongoTemplate.getCollection( clNames[ 1 ] );

        // 做简单的增删改查操作
        crud( cl1 );
        crud( cl2 );

        // 刪除集合
        for ( String clName : clNames ) {
            mongoTemplate.dropCollection( clName );
        }
        for ( String clName : clNames ) {
            Assert.assertFalse( mongoTemplate.collectionExists( clName ) );
        }
    }

    @Test
    public void test2() throws UnknownHostException {
        String dbName = springDBNameWithVersion + "_db21924";
        String clName = springDBNameWithVersion + "_cl21924_test2";
        MongoTemplate mongoTemplate = new MongoTemplate( client, dbName );
        // cs和cl都不存在，执行insert、upsert、createIndex、createCL 操作,创建cs和cl
        autoCreate( mongoTemplate, clName );
        // cs存在，cl不存在，执行insert、upsert、createIndex、createCL操作
        mongoTemplate.createCollection( clName ).drop();
        autoCreate( mongoTemplate, clName );
    }

    private void autoCreate( MongoTemplate mongoTemplate, String clName ) {
        // insert
        mongoTemplate.insert( new BasicDBObject( "a", 1 ), clName );
        Assert.assertTrue( mongoTemplate.collectionExists( clName ) );
        mongoTemplate.getDb().dropDatabase();

        // upsert
        Assert.assertFalse( mongoTemplate.collectionExists( clName ) );
        mongoTemplate.upsert( new Query( Criteria.where( "a" ).is( 0 ) ),
                Update.update( "a", 1 ), clName );
        Assert.assertTrue( mongoTemplate.collectionExists( clName ) );
        mongoTemplate.getDb().dropDatabase();

        // createIndex
        Assert.assertFalse( mongoTemplate.collectionExists( clName ) );
        mongoTemplate.indexOps( clName ).ensureIndex( new Index().named( "a" )
                .unique().on( "a", Sort.Direction.ASC ) );
        Assert.assertTrue( mongoTemplate.collectionExists( clName ) );
        mongoTemplate.getDb().dropDatabase();

        // create cl
        Assert.assertFalse( mongoTemplate.collectionExists( clName ) );
        mongoTemplate.createCollection( clName );
        Assert.assertTrue( mongoTemplate.collectionExists( clName ) );
        mongoTemplate.getDb().dropDatabase();
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
        Assert.assertEquals( cl.count( new BasicDBObject( "a", num + 1 ) ), 1 );

        // 刪除
        WriteResult result4 = cl.remove( new BasicDBObject() );
        Assert.assertEquals( result4.getN(), num );
        Assert.assertEquals( cl.count( new BasicDBObject() ), 0 );
    }

    @AfterClass
    public void tearDown() {
    }
}
