package com.mongodb.m2s.analyzer;

import com.mongodb.MongoCommandException;
import com.mongodb.client.MongoClient;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoDatabase;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32758:存在多种类型集合，分析收集的集合信息文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/16
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32758 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32758";
    String commColl = "commColl";
    String rangeColl = "rangeColl";
    String hashedColl = "hashedColl";
    String compoundColl1 = "compoundColl1";
    String compoundColl2 = "compoundColl2";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        if ( !CommLib.isSharded( mongoClient ) ) {
            throw new SkipException( "this test is only for sharded cluster" );
        }
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        MongoDatabase adminDatabase = mongoClient.getDatabase( "admin" );
        // 创建集合并开启分片功能
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        adminDatabase
                .runCommand( new Document( "enableSharding", databaseName ) );

        // 创建普通集合
        database.createCollection( commColl );
        // 创建range分区集合
        database.createCollection( rangeColl );
        database.getCollection( rangeColl )
                .createIndex( new Document( "age", 1 ) );
        adminDatabase.runCommand( new Document( "shardCollection",
                databaseName + "." + rangeColl ).append( "key",
                        new Document( "age", 1 ) ) );
        // 创建hashed集合
        database.createCollection( hashedColl );
        database.getCollection( hashedColl )
                .createIndex( new Document( "age", "hashed" ) );
        adminDatabase.runCommand( new Document( "shardCollection",
                databaseName + "." + hashedColl ).append( "key",
                        new Document( "age", "hashed" ) ) );

        // 创建compound集合,4.2及之前的版本不支持复合索引包含hashed索引，4.4版本及之前版本复合索引不支持包含多个hashed索引
        database.createCollection( compoundColl1 );
        database.getCollection( compoundColl1 )
                .createIndex( new Document( "name", 1 ).append( "age", 1 ) );
        adminDatabase.runCommand( new Document( "shardCollection",
                databaseName + "." + compoundColl1 ).append( "key",
                        new Document( "name", 1 ).append( "age", 1 ) ) );

        database.createCollection( compoundColl2 );
        try {
            // Currently only single field hashed index supported.
            database.getCollection( compoundColl2 ).createIndex(
                    new Document( "name", 1 ).append( "age", "hashed" ) );
            adminDatabase.runCommand( new Document( "shardCollection",
                    databaseName + "." + compoundColl2 ).append( "key",
                            new Document( "name", 1 ).append( "age",
                                    "hashed" ) ) );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() == 16763 ) {
                database.getCollection( compoundColl2 ).drop();
            } else {
                throw e;
            }
        }

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        // 分析集合信息
        String collectionJsonPath = collectorOutputPath + "collection.json";
        CommLib.analyzeCollection( ssh, collectionJsonPath );

        // 分析结果校验
        ssh.exec( "cat " + analyzerOutputPath + "collection.json" );
        // 复合分区包含多种类型索引的显示不兼容
        JSONArray collections = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < collections.size(); i++ ) {
            JSONObject collection = collections.getJSONObject( i );
            if ( collection.getString( "collection" )
                    .equals( databaseName + "." + commColl ) ) {
                Assert.assertNull( collection.get( "shardingKey" ) );
                Assert.assertNull( collection.get( "incompatible" ) );
            }
            if ( collection.getString( "collection" )
                    .equals( databaseName + "." + rangeColl ) ) {
                String expected = new Document( "age", 1 ).toJson();
                Assert.assertEquals( collection.getJSONObject( "shardingKey" ),
                        JSONObject.parseObject( expected ) );
                Assert.assertNull( collection.get( "incompatible" ) );
            }
            if ( collection.getString( "collection" )
                    .equals( databaseName + "." + hashedColl ) ) {
                String expected = new Document( "age", "hashed" ).toJson();
                Assert.assertEquals( collection.getJSONObject( "shardingKey" ),
                        JSONObject.parseObject( expected ) );
                Assert.assertNull( collection.get( "incompatible" ) );
            }
            if ( collection.getString( "collection" )
                    .equals( databaseName + "." + compoundColl1 ) ) {
                String expected = new Document( "name", 1 ).append( "age", 1 )
                        .toJson();
                Assert.assertEquals( collection.getJSONObject( "shardingKey" ),
                        JSONObject.parseObject( expected ) );
                Assert.assertNull( collection.get( "incompatible" ) );
            }
            if ( collection.getString( "collection" )
                    .equals( databaseName + "." + compoundColl2 ) ) {
                String expected = new Document( "name", 1 )
                        .append( "age", "hashed" ).toJson();
                Assert.assertEquals( collection.get( "shardingKey" ),
                        JSONObject.parseObject( expected ) );
                Assert.assertTrue( collection.getJSONArray( "incompatible" )
                        .contains( "shardingType" ) );
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
