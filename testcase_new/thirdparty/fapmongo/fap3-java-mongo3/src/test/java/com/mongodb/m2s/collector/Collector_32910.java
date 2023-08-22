package com.mongodb.m2s.collector;

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
 * @Descreption seqDB-32910:使用自定义桶名，收集集群信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class Collector_32910 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName1 = "db_32910_1";
    private String databaseName2 = "db_32910_2";
    private String databaseName3 = "db_32910_3";
    private String bucketName = "bucket_32910";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName1 ).drop();
        mongoClient.getDatabase( databaseName2 ).drop();
        mongoClient.getDatabase( databaseName3 ).drop();
    }

    @Test
    public void test() throws Exception {
        // 开启gridfs，使用默认桶名
        MongoDatabase mongoDatabase1 = mongoClient.getDatabase( databaseName1 );
        mongoDatabase1.createCollection( "fs.files" );
        mongoDatabase1.createCollection( "fs.chunks" );

        // 开启gridfs，使用自定义桶名
        MongoDatabase mongoDatabase2 = mongoClient.getDatabase( databaseName2 );
        mongoDatabase2.createCollection( bucketName + "." + "files" );
        mongoDatabase2.createCollection( bucketName + "." + "chunks" );

        // 开启gridfs,同时存在默认桶和自定义桶
        MongoDatabase mongoDatabase3 = mongoClient.getDatabase( databaseName3 );
        mongoDatabase3.createCollection( "fs.files" );
        mongoDatabase3.createCollection( "fs.chunks" );
        mongoDatabase3.createCollection( bucketName + "." + "files" );
        mongoDatabase3.createCollection( bucketName + "." + "chunks" );

        // 收集集合信息
        CommLib.collectCluster( ssh );

        // 校验数据库信息
        int gridfsDBCount = 0;
        ssh.exec( "cat " + collectorOutputPath + "cluster.json" );
        JSONObject cluster = JSONObject.parseObject( ssh.getStdout() );
        JSONArray databases = cluster.getJSONArray( "databases" );
        for ( int i = 0; i < databases.size(); i++ ) {
            JSONObject database = databases.getJSONObject( i );
            String name = database.getString( "name" );
            if ( name.equals( databaseName1 ) || name.equals( databaseName2 )
                    || name.equals( databaseName3 ) ) {
                Assert.assertTrue( database.getBoolean( "gridfs" ) );
                gridfsDBCount++;
            }
        }
        Assert.assertEquals( gridfsDBCount, 3 );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName1 ).drop();
        mongoClient.getDatabase( databaseName2 ).drop();
        mongoClient.getDatabase( databaseName3 ).drop();
        if ( mongoClient != null ) {
            mongoClient.close();
        }
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }

}
