package com.mongodb.m2s.collector;

import org.bson.BsonArray;
import org.bson.BsonDouble;
import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.IndexOptions;
import com.mongodb.client.model.Indexes;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32667:工具收集index地理空间索引与哈希索引信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class Collector_32667 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32667";
    private String collectionName = "col_32667";
    private final String indexName1 = "2dIndex";
    private final String indexName2 = "2dsphereIndex";
    private final String indexName3 = "geoHaystackIndex";
    private final String indexName4 = "hashedIndex";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        // 创建集合并插入数据
        MongoDatabase mongoDatabase = mongoClient.getDatabase( databaseName );
        MongoCollection< Document > collection = mongoDatabase
                .getCollection( collectionName );
        BsonArray location1 = new BsonArray();
        location1.add( new BsonDouble( 34.55 ) );
        location1.add( new BsonDouble( -34.55 ) );
        Document location2 = new Document( "type", "Point" )
                .append( "coordinates", location1 );
        for ( int i = 0; i < 50; i++ ) {
            Document document = new Document( "2dField", location1 )
                    .append( "hashedField", i )
                    .append( "2dsphereField", location2 )
                    .append( "geoHaystackField", location2 )
                    .append( "category", i );
            collection.insertOne( document );
        }

        // 创建2d索引
        collection.createIndex( Indexes.geo2d( "2dField" ),
                new IndexOptions().name( indexName1 ) );
        // 创建2dsphere索引
        collection.createIndex( Indexes.geo2dsphere( "2dsphereField" ),
                new IndexOptions().name( indexName2 ) );
        // 创建geoHaystack索引
        collection.createIndex(
                Indexes.geoHaystack( "geoHaystackField" + "." + "coordinates",
                        new Document( "category", 1 ) ),
                new IndexOptions().name( indexName3 ).bucketSize( 1.0 ) );
        // 创建哈希索引
        collection.createIndex( Indexes.hashed( "hashedField" ),
                new IndexOptions().name( indexName4 ) );

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        boolean contain2dIndex = false;
        boolean contain2dsphereIndex = false;
        boolean containGeoHaystackIndex = false;
        boolean containHashedIndex = false;
        // 校验集合信息
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections1 = ssh.getStdout().split( "\n" );
        for ( String str : collections1 ) {
            JSONObject collectionJson = JSONObject.parseObject( str );
            if ( collectionJson.getString( "database" )
                    .equals( databaseName ) ) {
                if ( collectionJson.getString( "name" )
                        .equals( collectionName ) ) {
                    JSONArray indexes = collectionJson
                            .getJSONArray( "indexes" );
                    for ( int i = 0; i < indexes.size(); i++ ) {
                        JSONObject index = indexes.getJSONObject( i );
                        String indexName = index.getString( "name" );
                        switch ( indexName ) {
                        case indexName1:
                            contain2dIndex = true;
                            String expectKey = new Document( "2dField", "2d" )
                                    .toJson();
                            Assert.assertEquals( index.getJSONObject( "key" ),
                                    JSONObject.parseObject( expectKey ) );
                            break;
                        case indexName2:
                            contain2dsphereIndex = true;
                            expectKey = new Document( "2dsphereField",
                                    "2dsphere" ).toJson();
                            Assert.assertEquals( index.getJSONObject( "key" ),
                                    JSONObject.parseObject( expectKey ) );
                            break;
                        case indexName3:
                            containGeoHaystackIndex = true;
                            expectKey = new Document(
                                    "geoHaystackField.coordinates",
                                    "geoHaystack" ).append( "category", 1 )
                                            .toJson();
                            Assert.assertEquals( index.getJSONObject( "key" ),
                                    JSONObject.parseObject( expectKey ) );
                            break;
                        case indexName4:
                            containHashedIndex = true;
                            expectKey = new Document( "hashedField", "hashed" )
                                    .toJson();
                            Assert.assertEquals( index.getJSONObject( "key" ),
                                    JSONObject.parseObject( expectKey ) );
                            break;
                        default:
                            continue;
                        }
                    }
                }
                break;
            }
        }
        Assert.assertTrue( contain2dIndex );
        Assert.assertTrue( contain2dsphereIndex );
        Assert.assertTrue( containGeoHaystackIndex );
        Assert.assertTrue( containHashedIndex );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
        if ( mongoClient != null ) {
            mongoClient.close();
        }
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }

}
