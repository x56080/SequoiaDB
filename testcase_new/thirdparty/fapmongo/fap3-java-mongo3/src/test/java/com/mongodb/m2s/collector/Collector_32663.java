package com.mongodb.m2s.collector;

import com.mongodb.client.MongoCollection;
import com.mongodb.client.model.IndexOptions;
import com.mongodb.client.model.Indexes;
import org.bson.*;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoDatabase;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32663:工具收集index一般索引信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class Collector_32663 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32663";
    private String collectionName = "col_32663";
    private final String commonIndex1 = "commonIndex1";
    private final String commonIndex2 = "commonIndex2";
    private final String uniqueIndex = "uniqueIndex";
    private final String compoundIndex1 = "compoundIndex1";
    private final String compoundIndex2 = "compoundIndex2";
    boolean containCommonIndex1 = false;
    boolean containCommonIndex2 = false;
    boolean containUniqueIndex = false;
    boolean containCompoundIndex1 = false;
    boolean containCompoundIndex2 = false;

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
        for ( int i = 0; i < 50; i++ ) {
            Document document = new Document( "name", "test" + i )
                    .append( "age", i ).append( "part1", i )
                    .append( "part2", i ).append( "part3", i )
                    .append( "part4", i ).append( "high", i );
            collection.insertOne( document );
        }

        // 创建普通索引
        collection.createIndex( new Document( "name", 1 ),
                new IndexOptions().name( commonIndex1 ) );
        // 创建唯一索引
        collection.createIndex( new Document( "age", 1 ),
                new IndexOptions().unique( true ).name( uniqueIndex ) );
        // 创建复合索引
        collection.createIndex( new Document( "part1", 1 ).append( "part2", 1 ),
                new IndexOptions().name( compoundIndex1 ) );
        // 4.2及之前的版本不支持复合索引包含hashed索引
        if ( CommLib.compareVersion( CommLib.getMongoDBVersion( mongoClient ),
                "4.4" ) >= 0 ) {
            collection.createIndex(
                    Indexes.compoundIndex( Indexes.ascending( "part3" ),
                            Indexes.hashed( "part4" ) ),
                    new IndexOptions().name( compoundIndex2 ) );
        }

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        // 校验索引信息
        verifyIndexes();
        Assert.assertTrue( containCommonIndex1 );
        Assert.assertFalse( containCommonIndex2 );
        Assert.assertTrue( containUniqueIndex );
        Assert.assertTrue( containCompoundIndex1 );
        if ( CommLib.compareVersion( CommLib.getMongoDBVersion( mongoClient ),
                "4.4" ) >= 0 ) {
            Assert.assertTrue( containCompoundIndex2 );
        }

        // 再次创建一个普通索引后收集集合信息
        collection.createIndex( new Document( "high", 1 ),
                new IndexOptions().name( commonIndex2 ) );
        CommLib.collectCollection( ssh, collectSample );
        verifyIndexes();
        Assert.assertTrue( containCommonIndex2 );
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

    private void verifyIndexes() throws Exception {
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections = ssh.getStdout().split( "\n" );
        for ( String str : collections ) {
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
                        JSONObject key = index.getJSONObject( "key" );
                        switch ( indexName ) {
                        case commonIndex1:
                            String expectKey = new Document( "name", 1 )
                                    .toJson();
                            Assert.assertEquals( key,
                                    JSONObject.parseObject( expectKey ) );
                            containCommonIndex1 = true;
                            break;
                        case commonIndex2:
                            expectKey = new Document( "high", 1 ).toJson();
                            Assert.assertEquals( key,
                                    JSONObject.parseObject( expectKey ) );
                            containCommonIndex2 = true;
                            break;
                        case uniqueIndex:
                            expectKey = new Document( "age", 1 ).toJson();
                            Assert.assertEquals( key,
                                    JSONObject.parseObject( expectKey ) );
                            Assert.assertTrue( index.getBoolean( "unique" ) );
                            containUniqueIndex = true;
                            break;
                        case compoundIndex1:
                            expectKey = new Document( "part1", 1 )
                                    .append( "part2", 1 ).toJson();
                            Assert.assertEquals( key,
                                    JSONObject.parseObject( expectKey ) );
                            containCompoundIndex1 = true;
                            break;
                        case compoundIndex2:
                            expectKey = new Document( "part3", 1 )
                                    .append( "part4", "hashed" ).toJson();
                            Assert.assertEquals( key,
                                    JSONObject.parseObject( expectKey ) );
                            containCompoundIndex2 = true;
                            break;
                        default:
                            continue;
                        }
                    }
                    break;
                }
            }
        }
    }

}
