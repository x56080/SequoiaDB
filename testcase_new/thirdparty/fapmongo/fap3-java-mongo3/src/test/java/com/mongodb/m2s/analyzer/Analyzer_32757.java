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
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32757:存在collation不为null的集合，分析收集的集合信息文件
 * @Author      chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/15
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32757 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String db_32757 = "db_32757";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
    }

    @Test
    public void test() throws Exception {
        MongoDatabase database = mongoClient.getDatabase( db_32757 );
        String commcoll = "commColl";
        String collationColl = "collationColl";
        if ( CommLib.collectionExist( database, commcoll ) ) {
            database.getCollection( commcoll ).drop();
        }
        if ( CommLib.collectionExist( database, collationColl ) ) {
            database.getCollection( collationColl ).drop();
        }
        database.createCollection( commcoll );
        for ( int i = 0; i < 100; i++ ) {
            database.getCollection( commcoll )
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
        CommLib.collectCollection( ssh, collectorUri, collectorOutputPath );

        // 分析集合信息
        String collectionjsonPath = collectorOutputPath + "collection.json";
        CommLib.analyzeCollection( ssh, collectionjsonPath, analyzerUri,
                analyzerOutputPath, sdbVersion );

        // 分析结果校验
        ssh.exec( "cat " + analyzerOutputPath + "collection.json" );
        // collaton不为null的集合，incompatible字段中包含collation
        JSONArray collections = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < collections.size(); i++ ) {
            JSONObject collection = collections.getJSONObject( i );
            if ( collection.getString( "collection" )
                    .equals( db_32757 + "." + commcoll ) ) {
                Assert.assertNull( collection.get( "collation" ) );
                Assert.assertEquals( collection.get( "incompatible" ), null );
            }
            if ( collection.getString( "collection" )
                    .equals( db_32757 + "." + collationColl ) ) {
                Assert.assertNotNull( collection.get( "collation" ) );
                JSONArray incompatible = collection
                        .getJSONArray( "incompatible" );
                Assert.assertEquals( incompatible.size(), 1,
                        incompatible.toJSONString() );
                Assert.assertEquals( incompatible.contains( "collation" ), true,
                        incompatible.toJSONString() );
            }
        }

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        CommLib.rmDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( db_32757 ).drop();
        ssh.disconnect();
        mongoClient.close();
    }

}
