package com.mongodb.m2s.analyzer;

import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.*;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32759:存在固定集合和非固定集合，分析收集的集合信息文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/16
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32759 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32759";
    private String collectionName1 = "commColl";
    private String collectionName2 = "cappedColl";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        // 创建非固定集合
        database.createCollection( collectionName1 );
        for ( int i = 0; i < 100; i++ ) {
            database.getCollection( collectionName1 )
                    .insertOne( new Document( "name", "test" + i ) );
        }

        // 创建固定集合
        CreateCollectionOptions options = new CreateCollectionOptions();
        options.capped( true ).sizeInBytes( 1024 );
        database.createCollection( collectionName2, options );
        for ( int i = 0; i < 100; i++ ) {
            database.getCollection( collectionName2 )
                    .insertOne( new Document( "name", "test" + i ) );
        }

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        // 分析集合信息
        String collectionJsonPath = collectorOutputPath + "collection.json";
        CommLib.analyzeCollection( ssh, collectionJsonPath );

        // 分析结果校验
        ssh.exec( "cat " + analyzerOutputPath + "collection.json" );
        // 固定集合，incompatible字段中包含capped
        JSONArray collections = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < collections.size(); i++ ) {
            JSONObject collection = collections.getJSONObject( i );
            if ( collection.getString( "collection" )
                    .equals( databaseName + "." + collectionName1 ) ) {
                Assert.assertFalse( collection.getBoolean( "capped" ) );
                Assert.assertNull( collection.get( "incompatible" ) );
                continue;
            }
            if ( collection.getString( "collection" )
                    .equals( databaseName + "." + collectionName2 ) ) {
                Assert.assertTrue( collection.getBoolean( "capped" ) );
                JSONArray incompatible = collection
                        .getJSONArray( "incompatible" );
                Assert.assertEquals( incompatible.size(), 1,
                        incompatible.toJSONString() );
                Assert.assertTrue( incompatible.contains( "capped" ),
                        incompatible.toJSONString() );
                continue;
            }
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
