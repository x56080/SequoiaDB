package com.mongodb.m2s.collector;

import com.mongodb.client.MongoDatabase;
import org.bson.Document;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * @Descreption seqDB-33038:收集集群信息，自动跳过视图与system集合
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/31
 * @UpdateRemark
 * @Version
 */
public class Collector_33038 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_33038";
    private String collectionName = "col_33038";
    private String viewName = "view_33038";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        if ( CommLib.compareVersion( CommLib.getMongoDBVersion( mongoClient ),
                "3.4" ) < 0 ) {
            throw new SkipException( "this version is not support create view!!!" );
        }
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        // 创建集合并插入数据
        database.createCollection( collectionName );
        for ( int i = 0; i < 100; i++ ) {
            Document document = new Document( "name", "test" + i )
                    .append( "age", i );
            database.getCollection( collectionName ).insertOne( document );
        }
        // 创建视图
        database.createView( viewName, collectionName,
                Arrays.asList( new Document( "$match",
                        new Document( "age", new Document( "$gt", 50 ) ) ) ) );
        // 创建视图成功后会在数据库创建视图集合和system.views集合
        ArrayList< String > collectionNames1 = database.listCollectionNames()
                .into( new ArrayList<>() );
        Assert.assertTrue( collectionNames1.contains( viewName ) );
        Assert.assertTrue( collectionNames1.contains( "system.views" ) );

        // 创建相似集合名的集合
        database.createCollection( viewName + "_1" );
        database.createCollection( "systemtest" );

        CommLib.collectCollection( ssh, collectSample );

        // 检验集合信息
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        ArrayList< String > collectionName2 = new ArrayList<>();
        String[] split = ssh.getStdout().split( "\n" );
        for ( int i = 0; i < split.length; i++ ) {
            JSONObject collection = JSONObject.parseObject( split[ i ] );
            String databaseName = collection.getString( "database" );
            if ( databaseName.equals( databaseName ) ) {
                collectionName2.add( collection.getString( "name" ) );
            }
        }
        // 收集时跳过视图和以"system."开头的系统表
        Assert.assertTrue( collectionName2.contains( collectionName ) );
        Assert.assertTrue( !collectionName2.contains( viewName ) );
        Assert.assertTrue( collectionName2.contains( viewName + "_1" ) );
        Assert.assertTrue( collectionName2.contains( "systemtest" ) );
        Assert.assertTrue( !collectionName2.contains( "system.views" ) );
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
