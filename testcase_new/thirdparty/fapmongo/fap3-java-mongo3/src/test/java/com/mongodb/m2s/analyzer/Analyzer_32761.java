package com.mongodb.m2s.analyzer;

import com.mongodb.client.model.*;
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
 * @Descreption seqDB-32761:存在collation不为null的索引，分析收集的集合信息文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/17
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32761 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32761";
    private String collectionName = "col_32761";
    String commonIndex = "commonIndex";
    String collationIndex = "collationIndex";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        // 3.4版本开始支持创建collation不为null的索引
        String version = CommLib.getMongoDBVersion( mongoClient );
        if ( CommLib.compareVersion( version, "3.4" ) < 0 ) {
            throw new SkipException(
                    "this test is only for mongodb version >= 3.4" );
        }
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();

    }

    @Test
    public void test() throws Exception {
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        database.createCollection( collectionName );

        for ( int i = 0; i < 100; i++ ) {
            database.getCollection( collectionName ).insertOne(
                    new Document( "name", "test" + i ).append( "age", i ) );
        }

        // 不指定collation创建索引
        database.getCollection( collectionName ).createIndex(
                new Document( "name", 1 ),
                new IndexOptions().name( commonIndex ) );

        // 指定collation创建索引
        database.getCollection( collectionName ).createIndex(
                new Document( "age", 1 ),
                new IndexOptions().name( collationIndex ).collation( Collation
                        .builder().locale( "en_US" )
                        .collationStrength( CollationStrength.SECONDARY )
                        .caseLevel( true )
                        .collationCaseFirst( CollationCaseFirst.UPPER )
                        .collationAlternate( CollationAlternate.SHIFTED )
                        .collationMaxVariable( CollationMaxVariable.SPACE )
                        .backwards( true ).normalization( true )
                        .numericOrdering( true ).build() ) );

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        // 分析集合信息
        String collectionJsonPath = collectorOutputPath + "collection.json";
        CommLib.analyzeCollection( ssh, collectionJsonPath );

        // 分析结果校验index.json
        ssh.exec( "cat " + analyzerOutputPath + "index.json" );
        // 指定collation创建的索引，incompatible字段中包含collation
        JSONArray indexes = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < indexes.size(); i++ ) {
            JSONObject index = indexes.getJSONObject( i );
            if ( index.getString( "collection" )
                    .equals( databaseName + "." + collectionName )
                    && index.getString( "index" ).equals( commonIndex ) ) {
                String expected = new Document( "name", 1 ).toJson();
                Assert.assertEquals( index.getJSONObject( "key" ),
                        JSONObject.parseObject( expected ) );
                Assert.assertNull( index.get( "collation" ) );
                Assert.assertNull( index.get( "incompatible" ) );
            }
            if ( index.getString( "collection" )
                    .equals( databaseName + "." + collectionName )
                    && index.getString( "index" ).equals( collationIndex ) ) {
                String expected = new Document( "age", 1 ).toJson();
                Assert.assertEquals( index.getJSONObject( "key" ),
                        JSONObject.parseObject( expected ) );
                Assert.assertNotNull( index.get( "collation" ) );
                JSONArray incompatible = index.getJSONArray( "incompatible" );
                Assert.assertEquals( incompatible.size(), 1,
                        incompatible.toJSONString() );
                Assert.assertTrue( incompatible.contains( "collation" ),
                        incompatible.toJSONString() );
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
