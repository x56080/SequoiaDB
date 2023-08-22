package com.mongodb.m2s.collector;

import com.mongodb.client.model.CreateCollectionOptions;
import org.bson.Document;
import org.testng.Assert;
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
 * @Descreption seqDB-32661:工具收集collection固定集合信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Collector_32661 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32661";
    private String collectionName = "col_32661";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        database.createCollection( collectionName, new CreateCollectionOptions()
                .capped( true ).sizeInBytes( 1024 * 1024 ) );
        for ( int i = 0; i < 100; i++ ) {
            database.getCollection( collectionName )
                    .insertOne( new Document( "name", "test" + i ) );
        }

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        // 验证收集的信息,成功收集到固定集合，并且capped为true
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections2 = ssh.getStdout().split( "\n" );
        boolean isCollected = false;
        for ( String i : collections2 ) {
            JSONObject collection = JSONObject.parseObject( i );
            if ( collection.getString( "database" ).equals( databaseName )
                    && collection.getString( "name" )
                            .equals( collectionName ) ) {
                isCollected = true;
                Assert.assertTrue( collection.getBoolean( "capped" ) );
                Assert.assertEquals( collection.getIntValue( "count" ), 100 );
                break;
            }
        }
        Assert.assertTrue( isCollected );
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
