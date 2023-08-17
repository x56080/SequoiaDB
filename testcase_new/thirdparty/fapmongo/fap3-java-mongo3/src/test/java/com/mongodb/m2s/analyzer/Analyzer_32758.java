package com.mongodb.m2s.analyzer;

import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.*;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32758:存在多种类型集合，分析收集的集合信息文件
 * @Author      chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/15
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32758 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String db_32758 = "db_32758";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
    }

    @Test
    public void test() throws Exception {
        String commcoll = "commColl";
        String rangeColl = "rangeColl";
        String hashedColl = "hashedColl";
        String compoundColl = "compoundColl";
        mongoClient.getDatabase( db_32758 ).drop();

        MongoDatabase database = mongoClient.getDatabase( db_32758 );
        // 创建普通集合
        database.createCollection( commcoll );
        // 创建range分区集合


        // 创建hashed集合
        database.createCollection( hashedColl,
                new CreateCollectionOptions().storageEngineOptions(
                        new Document( "wiredTiger", new Document( "configString",
                                "block_compressor=zlib" ) ) ) );
        // 创建compound集合
        database.createCollection( compoundColl,
                new CreateCollectionOptions().storageEngineOptions(
                        new Document( "wiredTiger", new Document( "configString",
                                "block_compressor=zlib" ) ) ).capped( true )
                        .sizeInBytes( 1024 ).maxDocuments( 100 ) );


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
                    .equals( db_32758 + "." + commcoll ) ) {
                Assert.assertNull( collection.get( "collation" ) );
                Assert.assertEquals( collection.get( "incompatible" ), null );
            }
        }

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        CommLib.rmDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( db_32758 ).drop();
        if ( mongoClient != null )
            mongoClient.close();
        if ( ssh != null )
            ssh.disconnect();
    }

}
