package com.mongodb.m2s.analyzer;

import com.mongodb.client.model.IndexOptions;
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
 * @Descreption seqDB-32763:不同集合存在同名索引，分析收集的集合信息文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32763 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32763";
    String collectionName1 = "col_32763_1";
    String collectionName2 = "col_32763_2";
    String indexName = "name_1";

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
        database.createCollection( collectionName1 );
        database.createCollection( collectionName2 );

        // 插入数据
        for ( int i = 0; i < 100; i++ ) {
            database.getCollection( collectionName1 )
                    .insertOne( new Document( "name", "test" + i ) );
            database.getCollection( collectionName2 )
                    .insertOne( new Document( "name", "test" + i ) );
        }

        // 创建同名索引
        database.getCollection( collectionName1 ).createIndex(
                new Document( "name", 1 ),
                new IndexOptions().name( indexName ) );
        database.getCollection( collectionName2 ).createIndex(
                new Document( "name", 1 ),
                new IndexOptions().name( indexName ) );

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        // 分析集合信息
        String collectionJsonPath = collectorOutputPath + "collection.json";
        CommLib.analyzeCollection( ssh, collectionJsonPath );

        // 分析结果校验index.json
        ssh.exec( "cat " + analyzerOutputPath + "index.json" );
        boolean containIndex1 = false;
        boolean containIndex2 = false;
        // 可以正确分析到同名索引
        JSONArray indexes = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < indexes.size(); i++ ) {
            JSONObject index = indexes.getJSONObject( i );
            String collectionName = index.getString( "collection" );
            String indexName = index.getString( "index" );
            String expected = new Document( "name", 1 ).toJson();
            if ( collectionName.equals( databaseName + "." + collectionName1 )
                    && indexName.equals( this.indexName ) ) {
                Assert.assertNull( index.get( "incompatible" ) );
                Assert.assertEquals( index.getJSONObject( "key" ),
                        JSONObject.parseObject( expected ) );
                containIndex1 = true;
            }
            if ( collectionName.equals( databaseName + "." + collectionName2 )
                    && indexName.equals( this.indexName ) ) {
                Assert.assertNull( index.get( "incompatible" ) );
                Assert.assertEquals( index.getJSONObject( "key" ),
                        JSONObject.parseObject( expected ) );
                containIndex2 = true;
            }
        }
        Assert.assertTrue( containIndex1 );
        Assert.assertTrue( containIndex2 );

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
