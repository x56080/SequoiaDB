package com.mongodb.m2s.collector;

import com.alibaba.fastjson.JSONArray;
import org.bson.Document;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.*;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32730:工具收集index索引的核对信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class Collector_32730 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32730";
    private String collectionName = "col_32730";
    private String indexName = "index_32730";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        // 3.4版本开始支持指定collation
        if ( CommLib.compareVersion( CommLib.getMongoDBVersion( mongoClient ),
                "3.4" ) < 0 ) {
            throw new SkipException(
                    "skip test, this version is not support collation" );
        }
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        // 创建集合并插入数据
        MongoDatabase mongoDatabase = mongoClient.getDatabase( databaseName );

        mongoDatabase.createCollection( collectionName );
        for ( int i = 0; i < 50; i++ ) {
            Document document = new Document( "name", "test" + i )
                    .append( "age", i );
            mongoDatabase.getCollection( collectionName ).insertOne( document );
        }

        // 创建索引，指定collation
        mongoDatabase.getCollection( collectionName )
                .createIndex( Indexes.ascending( "name" ), new IndexOptions()
                        .collation( Collation.builder().locale( "en_US" )
                                .caseLevel( true )
                                .collationCaseFirst( CollationCaseFirst.LOWER )
                                .collationAlternate(
                                        CollationAlternate.SHIFTED )
                                .collationStrength(
                                        CollationStrength.SECONDARY )
                                .numericOrdering( true ).backwards( true )
                                .collationMaxVariable(
                                        CollationMaxVariable.SPACE )
                                .build() )
                        .name( indexName ) );

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        // 校验集合信息
        boolean collationIsNotNull = false;
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections = ssh.getStdout().split( "\n" );
        for ( String str : collections ) {
            JSONObject collectionJson = JSONObject.parseObject( str );
            if ( collectionJson.getString( "database" )
                    .equals( databaseName ) ) {
                if ( collectionJson.getString( "name" )
                        .equals( collectionName ) ) {
                    JSONArray indexes = collectionJson
                            .getJSONArray( "indexes" );
                    for ( int i = 0; i < indexes.size(); i++ ) {
                        JSONObject index = indexes.getJSONObject( i );
                        if ( index.getString( "name" ).equals( indexName ) ) {
                            JSONObject collation = index
                                    .getJSONObject( "collation" );
                            Assert.assertEquals(
                                    collation.getString( "locale" ), "en_US" );
                            Assert.assertTrue(
                                    collation.getBoolean( "caseLevel" ) );
                            Assert.assertEquals(
                                    collation.getString( "caseFirst" ),
                                    "lower" );
                            Assert.assertEquals(
                                    collation.getString( "alternate" ),
                                    "shifted" );
                            Assert.assertEquals(
                                    collation.getIntValue( "strength" ), 2 );
                            Assert.assertTrue(
                                    collation.getBoolean( "numericOrdering" ) );
                            Assert.assertTrue(
                                    collation.getBoolean( "backwards" ) );
                            Assert.assertEquals(
                                    collation.getString( "maxVariable" ),
                                    "space" );
                            collationIsNotNull = true;
                        }
                    }
                }
                break;
            }
        }
        Assert.assertTrue( collationIsNotNull );
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

}
