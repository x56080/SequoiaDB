package com.mongodb.m2s.collector;

import com.mongodb.client.MongoCollection;
import org.bson.Document;
import org.testng.Assert;
import org.testng.SkipException;
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
 * @Descreption seqDB-32658:工具收集database中数据库分片信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Collector_32658 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32658";
    private String collectionName = "col_32658";

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
        CommLib.collectCluster( ssh );
        ssh.exec( "cat " + collectorOutputPath + "cluster.json" );
        JSONObject cluster1 = JSONObject.parseObject( ssh.getStdout() );
        JSONArray databases1 = JSONObject
                .parseArray( cluster1.getString( "databases" ) );
        // 为分片时shards字段仅包含一个shard
        for ( int i = 0; i < databases1.size(); i++ ) {
            JSONObject database = databases1.getJSONObject( i );
            if ( database.getString( "name" ).equals( databaseName ) ) {
                JSONObject shards = JSONObject
                        .parseObject( database.getString( "shards" ) );
                Assert.assertEquals( shards.size(), 1 );
            }
        }

        // 开启分片
        adminDatabase
                .runCommand( new Document( "enableSharding", databaseName ) );
        mongoDatabase.getCollection( databaseName )
                .createIndex( new Document( "name", "hashed" ) );
        adminDatabase.runCommand( new Document( "shardCollection",
                databaseName + "." + collectionName ).append( "key",
                        new Document( "name", "hashed" ) ) );
        for ( int i = 0; i < 100; i++ ) {
            mongoDatabase.getCollection( collectionName )
                    .insertOne( new Document( "name", "test" + i ) );
        }
        // 等待分片完成，超时未收集到分片信息则失败
        int count = 0;
        while ( true ) {
            if ( count > 20 ) {
                Assert.fail( "sharding failed" );
            }
            Thread.sleep( 1000 );
            CommLib.collectCluster( ssh );
            ssh.exec( "cat " + collectorOutputPath + "cluster.json" );
            JSONObject cluster2 = JSONObject.parseObject( ssh.getStdout() );
            JSONArray databases2 = JSONObject
                    .parseArray( cluster2.getString( "databases" ) );
            int shardsSize = 0;
            for ( int i = 0; i < databases2.size(); i++ ) {
                JSONObject database = databases2.getJSONObject( i );
                if ( database.getString( "name" ).equals( databaseName ) ) {
                    JSONObject shards = JSONObject
                            .parseObject( database.getString( "shards" ) );
                    shardsSize = shards.size();
                    break;
                }
            }
            if ( shardsSize > 1 ) {
                break;
            }
            count++;
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
