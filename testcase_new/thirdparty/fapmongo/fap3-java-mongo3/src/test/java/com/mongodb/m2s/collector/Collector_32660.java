package com.mongodb.m2s.collector;

import org.bson.Document;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32660:工具收集collection分片集合信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Collector_32660 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32660";
    private String collectionName = "col_32660";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        if ( !CommLib.isSharded( mongoClient )
                || getShardNum( mongoClient ) < 2 ) {
            throw new SkipException(
                    "this test is only for sharded cluster with at least 2 shards" );
        }
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        MongoDatabase mongoDatabase = mongoClient.getDatabase( databaseName );
        MongoDatabase adminDatabase = mongoClient.getDatabase( "admin" );
        for ( int i = 0; i < 100; i++ ) {
            mongoDatabase.getCollection( collectionName )
                    .insertOne( new Document( "name", "test" + i ) );
        }

        // 为开启分片时分块数量小于1，shardKey为null
        CommLib.collectCollection( ssh, collectSample );
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections1 = ssh.getStdout().split( "\n" );
        for ( String i : collections1 ) {
            JSONObject collection = JSONObject.parseObject( i );
            if ( collection.getString( "database" ).equals( databaseName )
                    && collection.getString( "name" )
                            .equals( collectionName ) ) {
                Assert.assertEquals( collection.getIntValue( "count" ), 100 );
                Assert.assertTrue( collection.getIntValue( "nchunks" ) <= 1,
                        collection.toJSONString() );
                Assert.assertNull( collection.get( "shardKey" ) );
                break;
            }
        }

        // 开启分片并插入数据
        adminDatabase
                .runCommand( new Document( "enableSharding", databaseName ) );
        mongoDatabase.getCollection( collectionName )
                .createIndex( new Document( "name", "hashed" ) );
        adminDatabase.runCommand( new Document( "shardCollection",
                databaseName + "." + collectionName ).append( "key",
                        new Document( "name", "hashed" ) ) );
        for ( int i = 0; i < 100; i++ ) {
            mongoDatabase.getCollection( collectionName )
                    .insertOne( new Document( "name", "test" + i ) );
        }

        // 开启分片后分块数量大于0，shardKey不为空
        CommLib.collectCollection( ssh, collectSample );
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections2 = ssh.getStdout().split( "\n" );
        for ( String i : collections2 ) {
            JSONObject collection = JSONObject.parseObject( i );
            if ( collection.getString( "database" ).equals( databaseName )
                    && collection.getString( "name" )
                            .equals( collectionName ) ) {
                Assert.assertEquals( collection.getIntValue( "count" ), 200 );
                Assert.assertTrue( collection.getIntValue( "nchunks" ) >= 1,
                        collection.toJSONString() );
                Assert.assertEquals( collection.getJSONObject( "shardKey" ),
                        JSONObject.parseObject(
                                new Document( "name", "hashed" ).toJson() ) );
                break;
            }
        }
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

    public long getShardNum( MongoClient mongoClient ) {
        MongoCollection< Document > collection = mongoClient
                .getDatabase( "config" ).getCollection( "shards" );
        return collection.countDocuments();
    }
}
