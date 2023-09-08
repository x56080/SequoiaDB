package com.mongodb.m2s.analyzer;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.*;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32757:存在collation不为null的集合，分析收集的集合信息文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/15
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32757 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32757";
    String commColl = "commColl";
    String collationColl = "collationColl";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        // 3.4版本开始支持创建集合指定collation
        if ( CommLib.compareVersion( CommLib.getMongoDBVersion( mongoClient ),
                "3.4" ) < 0 ) {
            throw new SkipException(
                    "skip test, this version is not support collation" );
        }
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        database.createCollection( commColl );
        for ( int i = 0; i < 100; i++ ) {
            database.getCollection( commColl )
                    .insertOne( new Document( "name", "test" + i ) );
        }
        CreateCollectionOptions options = new CreateCollectionOptions();
        options.collation(
                Collation.builder().locale( "en_US" ).caseLevel( true )
                        .collationCaseFirst( CollationCaseFirst.UPPER )
                        .collationStrength( CollationStrength.SECONDARY )
                        .numericOrdering( false )
                        .collationAlternate( CollationAlternate.SHIFTED )
                        .collationMaxVariable( CollationMaxVariable.PUNCT )
                        .normalization( false ).backwards( false ).build() );
        database.createCollection( collationColl, options );
        for ( int i = 0; i < 100; i++ ) {
            database.getCollection( collationColl )
                    .insertOne( new Document( "name", "test" + i ) );
        }

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        // 分析集合信息
        String collectionJsonPath = collectorOutputPath + "collection.json";
        CommLib.analyzeCollection( ssh, collectionJsonPath );

        // 分析结果校验
        ssh.exec( "cat " + analyzerOutputPath + "collection.json" );
        // collation不为null的集合，incompatible字段中包含collation
        JSONArray collections = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < collections.size(); i++ ) {
            JSONObject collection = collections.getJSONObject( i );
            Assert.assertEquals( collection.getIntValue( "count" ), 100 );
            String collectionName = collection.getString( "collection" );
            if ( collectionName.equals( databaseName + "." + commColl ) ) {
                Assert.assertNull( collection.get( "collation" ) );
                Assert.assertNull( collection.get( "incompatible" ) );
            }
            if ( collectionName.equals( databaseName + "." + collationColl ) ) {
                Assert.assertNotNull( collection.get( "collation" ) );
                JSONArray incompatible = collection
                        .getJSONArray( "incompatible" );
                Assert.assertEquals( incompatible.size(), 1,
                        incompatible.toJSONString() );
                Assert.assertTrue( incompatible.contains( "collation" ),
                        incompatible.toJSONString() );
            }
        }
        // 汇总报告集合数量准确
        ssh.exec( "cat " + analyzerOutputPath + "summary.json" );
        JSONObject summary = JSONObject.parseObject( ssh.getStdout() );
        int collectionNum = summary.getIntValue( "collectionNum" );
        Assert.assertEquals( collectionNum, collections.size(), collectionNum );

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        CommLib.rmDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
        if ( mongoClient != null ) {
            mongoClient.close();
        }
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }

}
