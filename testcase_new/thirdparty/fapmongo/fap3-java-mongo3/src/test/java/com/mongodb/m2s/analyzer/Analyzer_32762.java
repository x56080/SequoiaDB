package com.mongodb.m2s.analyzer;

import com.mongodb.client.*;
import org.bson.BsonArray;
import org.bson.BsonDouble;
import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.model.*;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * @Descreption seqDB-32762:覆盖所有索引类型，分析收集的集合信息文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/18
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32762 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32762";
    private String collectionName = "col_32762";
    private String mongodbVersion = null;

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
        mongodbVersion = CommLib.getMongoDBVersion( mongoClient );
    }

    @Test
    public void test() throws Exception {
        List< String > allTypeIndex = new ArrayList<>();
        allTypeIndex.add( "singleIndex" );
        allTypeIndex.add( "hashedIndex" );
        allTypeIndex.add( "compoundIndex1" );
        allTypeIndex.add( "compoundIndex2" );
        allTypeIndex.add( "multiIndex" );
        allTypeIndex.add( "partialIndex" );
        allTypeIndex.add( "sparseIndex" );
        allTypeIndex.add( "ttlIndex" );
        allTypeIndex.add( "uniqueIndex" );
        allTypeIndex.add( "textIndex" );
        allTypeIndex.add( "wildcardIndex" );
        allTypeIndex.add( "2dIndex" );
        allTypeIndex.add( "2dsphereIndex" );
        allTypeIndex.add( "geoHaystackIndex" );

        MongoDatabase database = mongoClient.getDatabase( databaseName );
        database.createCollection( collectionName );
        MongoCollection< Document > collection = database
                .getCollection( collectionName );
        BsonArray location1 = new BsonArray();
        location1.add( new BsonDouble( 34.55 ) );
        location1.add( new BsonDouble( -34.55 ) );
        Document location2 = new Document( "type", "Point" )
                .append( "coordinates", location1 );
        for ( int i = 0; i < 100; i++ ) {
            collection.insertOne( new Document( "singleField", "test" + i )
                    .append( "hashedField", "test" + i )
                    .append( "compoundPart1", "part1" )
                    .append( "compoundPart2", "part2" )
                    .append( "compoundPart3", "part3" )
                    .append( "compoundPart4", "part4" )
                    .append( "multiField", new Document( "a", "a" ) )
                    .append( "partialField", "test" + i )
                    .append( "sparseField", "test" + i )
                    .append( "ttlField", "test" + i )
                    .append( "uniqueField", "test" + i )
                    .append( "textField", "test" + i )
                    .append( "wildcardField",
                            new Document( "a", "a" ).append( "b", "b" ) )
                    .append( "2dField", location1 )
                    .append( "2dsphereField", location2 )
                    .append( "geoHaystackField", location2 )
                    .append( "category", "test" + i ) );
        }

        // 创建单键索引
        collection.createIndex( Indexes.ascending( "singleField" ),
                new IndexOptions().name( "singleIndex" ) );
        // 创建哈希索引
        collection.createIndex( Indexes.hashed( "hashedField" ),
                new IndexOptions().name( "hashedIndex" ) );
        // 创建复合索引
        collection.createIndex(
                Indexes.compoundIndex( Indexes.ascending( "compoundPart1" ),
                        Indexes.ascending( "compoundPart2" ) ),
                new IndexOptions().name( "compoundIndex1" ) );
        // 4.2及之前的版本不支持复合索引包含hashed索引
        if ( CommLib.compareVersion( mongodbVersion, "4.4" ) >= 0 ) {
            collection.createIndex(
                    Indexes.compoundIndex( Indexes.ascending( "compoundPart3" ),
                            Indexes.hashed( "compoundPart4" ) ),
                    new IndexOptions().name( "compoundIndex2" ) );
        }
        // 创建多键索引
        collection.createIndex( Indexes.ascending( "multiField.a" ),
                new IndexOptions().name( "multiIndex" ) );
        // 创建部分索引
        collection.createIndex( Indexes.ascending( "partialField" ),
                new IndexOptions()
                        .partialFilterExpression(
                                Filters.eq( "partialField", "test1" ) )
                        .name( "partialIndex" ) );
        // 创建稀疏索引
        collection.createIndex( Indexes.ascending( "sparseField" ),
                new IndexOptions().sparse( true ).name( "sparseIndex" ) );
        // 创建TTL索引
        collection.createIndex( Indexes.ascending( "ttlField" ),
                new IndexOptions().expireAfter( 1L, TimeUnit.SECONDS )
                        .name( "ttlIndex" ) );
        // 创建唯一索引
        collection.createIndex( Indexes.ascending( "uniqueField" ),
                new IndexOptions().unique( true ).name( "uniqueIndex" ) );
        // 创建全文索引
        collection.createIndex( Indexes.text( "textField" ),
                new IndexOptions().name( "textIndex" ) );
        // 4.2版本才开始支持通配符索引
        if ( CommLib.compareVersion( mongodbVersion, "4.2" ) >= 0 ) {
            // 创建通配符索引
            collection.createIndex( Indexes.ascending( "wildcardField.$**" ),
                    new IndexOptions().name( "wildcardIndex" ) );
        }
        // 创建2d索引
        collection.createIndex( Indexes.geo2d( "2dField" ),
                new IndexOptions().name( "2dIndex" ) );
        // 创建2dsphere索引
        collection.createIndex( Indexes.geo2dsphere( "2dsphereField" ),
                new IndexOptions().name( "2dsphereIndex" ) );
        // 创建geoHaystack索引
        collection.createIndex(
                Indexes.geoHaystack( "geoHaystackField" + "." + "coordinates",
                        new Document( "category", 1 ) ),
                new IndexOptions().name( "geoHaystackIndex" )
                        .bucketSize( 1.0 ) );

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        // 分析集合信息
        String collectionJsonPath = collectorOutputPath + "collection.json";
        CommLib.analyzeCollection( ssh, collectionJsonPath );

        // 分析结果校验index.json
        ssh.exec( "cat " + analyzerOutputPath + "index.json" );
        // 依次校验每个索引，并将索引从allTypeIndex中移除，最终判断是否全部收集校验到
        JSONArray indexes = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < indexes.size(); i++ ) {
            JSONObject index = indexes.getJSONObject( i );
            if ( index.getString( "collection" )
                    .equals( databaseName + "." + collectionName ) ) {
                String indexName = index.getString( "index" );
                switch ( indexName ) {
                case "singleIndex":
                    String expected = new Document( "singleField", 1 ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    Assert.assertNull( index.get( "incompatible" ) );
                    allTypeIndex.remove( "singleIndex" );
                    continue;
                case "hashedIndex":
                    expected = new Document( "hashedField", "hashed" ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    JSONArray incompatible = index
                            .getJSONArray( "incompatible" );
                    Assert.assertEquals( incompatible.size(), 1,
                            incompatible.toJSONString() );
                    Assert.assertTrue(
                            incompatible.contains( "indexType: hashed" ),
                            incompatible.toJSONString() );
                    allTypeIndex.remove( "hashedIndex" );
                    continue;
                case "compoundIndex1":
                    expected = new Document( "compoundPart1", 1 )
                            .append( "compoundPart2", 1 ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    Assert.assertNull( index.get( "incompatible" ) );
                    allTypeIndex.remove( "compoundIndex1" );
                    continue;
                case "compoundIndex2":
                    expected = new Document( "compoundPart3", 1 )
                            .append( "compoundPart4", "hashed" ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    incompatible = index.getJSONArray( "incompatible" );
                    Assert.assertEquals( incompatible.size(), 1,
                            incompatible.toJSONString() );
                    Assert.assertTrue(
                            incompatible.contains( "indexType: UNKNOWN" ),
                            incompatible.toJSONString() );
                    allTypeIndex.remove( "compoundIndex2" );
                    continue;
                case "multiIndex":
                    expected = new Document( "multiField.a", 1 ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    Assert.assertNull( index.get( "incompatible" ) );
                    allTypeIndex.remove( "multiIndex" );
                    continue;
                case "partialIndex":
                    expected = new Document( "partialField", 1 ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    incompatible = index.getJSONArray( "incompatible" );
                    Assert.assertEquals( incompatible.size(), 1,
                            incompatible.toJSONString() );
                    Assert.assertTrue(
                            incompatible.contains( "partialFilterExpression" ),
                            incompatible.toJSONString() );
                    expected = new Document( "partialField", "test1" ).toJson();
                    Assert.assertEquals(
                            index.getJSONObject( "partialFilterExpression" ),
                            JSONObject.parseObject( expected ) );
                    allTypeIndex.remove( "partialIndex" );
                    continue;
                case "wildcardIndex":
                    expected = new Document( "wildcardField.$**", 1 ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    incompatible = index.getJSONArray( "incompatible" );
                    Assert.assertEquals( incompatible.size(), 1,
                            incompatible.toJSONString() );
                    Assert.assertTrue(
                            incompatible.contains( "indexType: wildcard" ),
                            incompatible.toJSONString() );
                    allTypeIndex.remove( "wildcardIndex" );
                    continue;
                case "sparseIndex":
                    expected = new Document( "sparseField", 1 ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    Assert.assertTrue( index.getBoolean( "sparse" ) );
                    incompatible = index.getJSONArray( "incompatible" );
                    Assert.assertEquals( incompatible.size(), 1,
                            incompatible.toJSONString() );
                    Assert.assertTrue( incompatible.contains( "sparse" ),
                            incompatible.toJSONString() );
                    allTypeIndex.remove( "sparseIndex" );
                    continue;
                case "uniqueIndex":
                    expected = new Document( "uniqueField", 1 ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    Assert.assertTrue( index.getBoolean( "unique" ) );
                    Assert.assertNull( index.get( "incompatible" ) );
                    allTypeIndex.remove( "uniqueIndex" );
                    continue;
                case "ttlIndex":
                    expected = new Document( "ttlField", 1 ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    Assert.assertEquals( index.getIntValue( "ttl" ), 1 );
                    incompatible = index.getJSONArray( "incompatible" );
                    Assert.assertEquals( incompatible.size(), 1,
                            incompatible.toJSONString() );
                    Assert.assertTrue( incompatible.contains( "ttl" ),
                            incompatible.toJSONString() );
                    allTypeIndex.remove( "ttlIndex" );
                    continue;
                case "textIndex":
                    expected = new Document( "_fts", "text" )
                            .append( "_ftsx", 1 ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    Assert.assertNull( index.get( "incompatible" ) );
                    allTypeIndex.remove( "textIndex" );
                    continue;
                case "2dIndex":
                    expected = new Document( "2dField", "2d" ).toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    incompatible = index.getJSONArray( "incompatible" );
                    Assert.assertEquals( incompatible.size(), 1,
                            incompatible.toJSONString() );
                    Assert.assertTrue( incompatible.contains( "indexType: 2d" ),
                            incompatible.toJSONString() );
                    allTypeIndex.remove( "2dIndex" );
                    continue;
                case "2dsphereIndex":
                    expected = new Document( "2dsphereField", "2dsphere" )
                            .toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    incompatible = index.getJSONArray( "incompatible" );
                    Assert.assertEquals( incompatible.size(), 1,
                            incompatible.toJSONString() );
                    Assert.assertTrue(
                            incompatible.contains( "indexType: 2dsphere" ),
                            incompatible.toJSONString() );
                    allTypeIndex.remove( "2dsphereIndex" );
                    continue;
                case "geoHaystackIndex":
                    expected = new Document( "category", 1 )
                            .append( "geoHaystackField.coordinates",
                                    "geoHaystack" )
                            .toJson();
                    Assert.assertEquals( index.getJSONObject( "key" ),
                            JSONObject.parseObject( expected ) );
                    incompatible = index.getJSONArray( "incompatible" );
                    Assert.assertEquals( incompatible.size(), 1,
                            incompatible.toJSONString() );
                    Assert.assertTrue(
                            incompatible.contains( "indexType: UNKNOWN" ),
                            incompatible.toJSONString() );
                    allTypeIndex.remove( "geoHaystackIndex" );
                default:
                    break;
                }
            }
        }
        if ( CommLib.compareVersion( mongodbVersion, "4.4" ) >= 0 ) {
            Assert.assertEquals( allTypeIndex.size(), 0 );
        } else {
            Assert.assertEquals( allTypeIndex.size(), 2 );
            Assert.assertTrue( allTypeIndex.contains( "compoundIndex2" ) );
            Assert.assertTrue( allTypeIndex.contains( "wildcardIndex" ) );
        }
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        CommLib.rmDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
        if ( mongoClient != null )
            mongoClient.close();
        if ( ssh != null )
            ssh.disconnect();
    }

}
