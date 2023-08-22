package com.mongodb.m2s.collector;

import com.mongodb.client.model.*;
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
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32729:工具收集collection核对信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class Collector_32729 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32729";
    private String collectionName = "col_32729";

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

        CreateCollectionOptions options = new CreateCollectionOptions();
        options.collation( Collation.builder().locale( "en_US" )
                .caseLevel( true )
                .collationCaseFirst( CollationCaseFirst.UPPER )
                .collationStrength( CollationStrength.SECONDARY )
                .collationAlternate( CollationAlternate.SHIFTED ).build() );
        mongoDatabase.createCollection( collectionName, options );
        for ( int i = 0; i < 50; i++ ) {
            Document document = new Document( "name", "test" + i )
                    .append( "age", i );
            mongoDatabase.getCollection( collectionName ).insertOne( document );
        }

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
                    collationIsNotNull = true;
                    JSONObject collation = collectionJson
                            .getJSONObject( "collation" );
                    Assert.assertTrue( collation.getBoolean( "caseLevel" ) );
                    Assert.assertEquals( collation.getString( "locale" ),
                            "en_US" );
                    Assert.assertEquals( collation.getString( "caseFirst" ),
                            "upper" );
                    Assert.assertEquals( collation.getIntValue( "strength" ),
                            2 );
                    Assert.assertEquals( collation.getString( "alternate" ),
                            "shifted" );
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
