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

import java.util.ArrayList;
import java.util.List;

/**
 * @Descreption seqDB-32753:独立集群创建数据库，分析收集集群信息文件
 *              seqDB-32754:副本集集群创建数据库，分析收集的集群信息文件
 *              seqDB-32755:分片模式创建数据库，分析收集的集群信息文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/11
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32753_32754_32755 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String gridfsDB = "gridfsDB";
    private String commonDB = "commonDB";
    private String collectionName = "coll_32753_32755";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( gridfsDB ).drop();
        mongoClient.getDatabase( commonDB ).drop();
    }

    @Test
    public void test() throws Exception {
        // 创建开启gridfs的数据库
        MongoDatabase database1 = mongoClient.getDatabase( gridfsDB );
        database1.createCollection( "fs.files" );
        database1.createCollection( "fs.chunks" );

        // 创建不开启gridfs的数据库
        MongoDatabase database2 = mongoClient.getDatabase( commonDB );
        MongoCollection collection = database2.getCollection( collectionName );
        collection.insertOne( new Document( "name", "john" ) );

        // 使用collector收集集群信息
        CommLib.collectCluster( ssh );

        // 使用analyzer分析集群信息
        String clusterJsonPath = collectorOutputPath + "cluster.json";
        CommLib.analyzeCluster( ssh, clusterJsonPath );

        // 成功收集到gridfsDB和commonDB，gridfs字段准确
        ssh.exec( "cat " + analyzerOutputPath + "database.json" );
        JSONArray databases = JSONObject.parseArray( ssh.getStdout() );
        List< String > collectDB = new ArrayList<>();
        for ( int i = 0; i < databases.size(); i++ ) {
            JSONObject database = ( JSONObject ) databases.get( i );
            if ( database.getString( "database" ).equals( gridfsDB ) ) {
                collectDB.add( gridfsDB );
                Assert.assertTrue( database.getBoolean( "gridfs" ) );
            }
            if ( database.getString( "database" ).equals( commonDB ) ) {
                collectDB.add( commonDB );
                Assert.assertFalse( database.getBoolean( "gridfs" ) );
            }
        }
        Assert.assertEquals( collectDB.size(), 2, collectDB.toString() );

        String deployMode = null;
        if ( CommLib.isSharded( mongoClient ) ) {
            deployMode = "shards";
        } else if ( CommLib.isReplicaSet( mongoClient ) ) {
            deployMode = "replset";
        } else if ( CommLib.isStandalone( mongoClient ) ) {
            deployMode = "standalone";
        } else {
            Assert.fail( "unknown deploy mode" );
        }
        // 集群模式分析准确
        ssh.exec( "cat " + analyzerOutputPath + "cluster.json" );
        JSONObject cluster = JSONObject.parseObject( ssh.getStdout() );
        Assert.assertEquals( cluster.getString( "instMode" ), deployMode );

        ssh.exec( "cat " + analyzerOutputPath + "summary.json" );
        JSONObject summary = JSONObject.parseObject( ssh.getStdout() );
        Assert.assertEquals( summary.getString( "instMode" ), deployMode );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        CommLib.rmDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( gridfsDB ).drop();
        mongoClient.getDatabase( commonDB ).drop();
        if ( mongoClient != null ) {
            mongoClient.close();
        }
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
