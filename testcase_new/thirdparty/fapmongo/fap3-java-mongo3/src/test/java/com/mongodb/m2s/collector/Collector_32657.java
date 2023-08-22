package com.mongodb.m2s.collector;

import com.alibaba.fastjson.JSONArray;
import com.mongodb.client.MongoDatabase;
import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32657:工具收集database数据库相关信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Collector_32657 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName1 = "db_32657_1";
    private String databaseName2 = "db_32657_2";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName1 ).drop();
        mongoClient.getDatabase( databaseName2 ).drop();
    }

    @Test
    public void test() throws Exception {
        MongoDatabase mongodatabaseName1 = mongoClient
                .getDatabase( databaseName1 );
        mongodatabaseName1.createCollection( "t1" );
        for ( int i = 0; i < 100; i++ ) {
            mongodatabaseName1.getCollection( "t1" )
                    .insertOne( new Document( "name", "test" + i ) );
        }
        CommLib.collectCluster( ssh );
        ssh.exec( "cat " + collectorOutputPath + "cluster.json" );
        JSONObject cluster1 = JSONObject.parseObject( ssh.getStdout() );
        JSONArray databases1 = JSONObject
                .parseArray( cluster1.getString( "databases" ) );
        boolean containDB1 = false;
        for ( int i = 0; i < databases1.size(); i++ ) {
            JSONObject database = databases1.getJSONObject( i );
            if ( database.getString( "name" ).equals( databaseName1 ) ) {
                Assert.assertEquals( database.getString( "collectionNum" ),
                        "1" );
                Assert.assertFalse( database.getBoolean( "empty" ) );
                Assert.assertFalse( database.getBoolean( "gridfs" ) );
                containDB1 = true;
            }
        }
        Assert.assertTrue( containDB1 );

        // 再次创建数据库能够收集到
        MongoDatabase mongodatabaseName2 = mongoClient
                .getDatabase( databaseName2 );
        mongodatabaseName2.createCollection( "t1" );
        for ( int i = 0; i < 100; i++ ) {
            mongodatabaseName2.getCollection( "t1" )
                    .insertOne( new Document( "name", "test" + i ) );
        }
        CommLib.collectCluster( ssh );
        ssh.exec( "cat " + collectorOutputPath + "cluster.json" );
        JSONObject cluster2 = JSONObject.parseObject( ssh.getStdout() );
        JSONArray databases2 = JSONObject
                .parseArray( cluster2.getString( "databases" ) );
        boolean containDB2 = false;
        for ( int i = 0; i < databases2.size(); i++ ) {
            JSONObject database = databases2.getJSONObject( i );
            if ( database.getString( "name" ).equals( databaseName2 ) ) {
                Assert.assertEquals( database.getString( "collectionNum" ),
                        "1" );
                Assert.assertFalse( database.getBoolean( "empty" ) );
                Assert.assertFalse( database.getBoolean( "gridfs" ) );
                containDB2 = true;
            }
        }
        Assert.assertTrue( containDB2 );

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName1 ).drop();
        mongoClient.getDatabase( databaseName2 ).drop();
        if ( mongoClient != null ) {
            mongoClient.close();
        }
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
