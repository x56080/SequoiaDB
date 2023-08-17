package com.mongodb.m2s.analyzer;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32753:独立集群创建数据库，分析收集集群信息文件
 *              seqDB-32754:副本集集群创建数据库，分析收集的集群信息文件
 *              seqDB-32755:分片模式创建数据库，分析收集的集群信息文件
 * @Author      chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/11
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32753_32755 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private MongoDatabase gridfsDB = null;
    private MongoDatabase commonDB = null;

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
    }

    @Test
    public void test() throws Exception {
        // 创建开启gridfs的数据库
        gridfsDB = mongoClient.getDatabase( "gridfsDB" );
        if ( !CommLib.collectionExist( gridfsDB, "fs.files" ) ) {
            gridfsDB.createCollection( "fs.files" );
        }
        if ( !CommLib.collectionExist( gridfsDB, "fs.chunks" ) ) {
            gridfsDB.createCollection( "fs.chunks" );
        }
        // 创建不开启gridfs的数据库
        commonDB = mongoClient.getDatabase( "commonDB" );
        MongoCollection collection = commonDB
                .getCollection( "commonCollection" );
        collection.insertOne( new Document( "name", "john" ) );

        // 使用collector收集集群信息
        CommLib.collectCluster( ssh, collectorUri, collectorOutputPath );

        // 使用analyzer分析集群信息
        String clusterjsonPath = collectorOutputPath + "cluster.json";
        CommLib.analyzeCluster( ssh, clusterjsonPath, analyzerUri,
                analyzerOutputPath, sdbVersion );

        ssh.exec( "cat " + analyzerOutputPath + "database.json" );
        JSONArray databases = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < databases.size(); i++ ) {
            JSONObject database = ( JSONObject ) databases.get( i );
            if ( database.get( "database" ).equals( "gridfsDB" ) ) {
                Assert.assertEquals( database.get( "gridfs" ), true );
            }
            if ( database.get( "database" ).equals( "commonDB" ) ) {
                Assert.assertEquals( database.get( "gridfs" ), false );
            }
        }
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        CommLib.rmDir( ssh, analyzerOutputPath );
        gridfsDB.drop();
        commonDB.drop();
        ssh.disconnect();
        mongoClient.close();
    }
}
