package com.mongodb.m2s.collector;

import org.bson.Document;
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

import java.util.HashMap;
import java.util.Map;

/**
 * @Descreption seqDB-32921:收集集群信息，查看cluster.json文件totalSize字段
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class Collector_32921 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32921";
    private String collectionName = "col_32921";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        // 创建集合并插入数据
        MongoDatabase mongoDatabase = mongoClient.getDatabase( databaseName );
        mongoDatabase.createCollection( collectionName );
        for ( int i = 0; i < 1000; i++ ) {
            Document document = new Document( "name", "test" + i )
                    .append( "age", i );
            mongoDatabase.getCollection( collectionName ).insertOne( document );
        }

        // 执行listDatabases命令
        MongoDatabase adminDatabase = mongoClient.getDatabase( "admin" );
        Document listDatabases = adminDatabase
                .runCommand( new Document( "listDatabases", 1 ) );
        JSONObject jsonObject = JSONObject
                .parseObject( listDatabases.toJson() );
        Long totalSizeAct = jsonObject.getLong( "totalSize" );
        Map< String, JSONObject > map = new HashMap<>();
        JSONArray databasesAct = jsonObject.getJSONArray( "databases" );
        for ( int i = 0; i < databasesAct.size(); i++ ) {
            JSONObject database = databasesAct.getJSONObject( i );
            String name = database.getString( "name" );
            map.put( name, database );
        }

        // 收集集群信息
        CommLib.collectCluster( ssh );

        // 校验集群信息文件totalSize字段
        ssh.exec( "cat " + collectorOutputPath + "cluster.json" );
        JSONObject cluster = JSONObject.parseObject( ssh.getStdout() );
        Assert.assertEquals( totalSizeAct, cluster.getLong( "totalSize" ) );
        // 校验每个数据库的信息
        JSONArray databases = cluster.getJSONArray( "databases" );
        for ( int i = 0; i < databases.size(); i++ ) {
            JSONObject database = databases.getJSONObject( i );
            String name = database.getString( "name" );
            JSONObject databaseAct = map.get( name );
            Assert.assertEquals( databaseAct.getLong( "sizeOnDisk" ),
                    database.getLong( "sizeOnDisk" ) );
            Assert.assertEquals( databaseAct.getBoolean( "empty" ),
                    database.getBoolean( "empty" ) );
            Assert.assertEquals( databaseAct.getJSONObject( "shards" ),
                    database.getJSONObject( "shards" ) );
        }
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
