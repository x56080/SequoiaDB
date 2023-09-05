package com.mongodb.m2s.collector;

import java.util.ArrayList;
import java.util.Arrays;

import org.bson.Document;
import org.testng.Assert;
import org.testng.SkipException;
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
 * @Descreption seqDB-33040:用户赋予权限的方式不同，收集集群信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/9/4
 * @UpdateRemark
 * @Version
 */
public class Collector_33041 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_33041";
    private String collectionName = "col_33041";
    private String user1 = "user1_33041";
    private String user2 = "user2_33041";
    private String user3 = "user3_33041";
    private String password = "123456";

    @BeforeClass
    public void setup() throws Exception {
        // mongodb启动鉴权时才执行用例，目前通过判断mongodbUri中是否包含@来判断是否启动鉴权
        if ( !M2STestBase.mongodbUri.contains( "@" ) ) {
            throw new SkipException( "this testcase is for auth mode " );
        }
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        MongoDatabase adminDatabase = mongoClient.getDatabase( "admin" );
        MongoDatabase database = mongoClient.getDatabase( databaseName );

        // 创建集合并插入数据
        database.createCollection( collectionName );
        for ( int i = 0; i < 100; i++ ) {
            Document document = new Document( "name", "test" + i )
                    .append( "age", i );
            database.getCollection( collectionName ).insertOne( document );
        }
        // admin数据库下创建用户，包含readAnyDatabase,clusterMonitor角色
        adminDatabase.runCommand( new Document( "createUser", user1 )
                .append( "pwd", password ).append( "roles", Arrays
                        .asList( "readAnyDatabase", "clusterMonitor" ) ) );
        // admin数据库下创建用户，包含readAnyDatabase角色
        adminDatabase.runCommand( new Document( "createUser", user2 )
                .append( "pwd", password )
                .append( "roles", Arrays.asList( "readAnyDatabase" ) ) );
        // admin数据库下创建用户，包含clusterMonitor角色
        adminDatabase.runCommand(
                new Document( "createUser", user3 ).append( "pwd", password )
                        .append( "roles", Arrays.asList( "clusterMonitor" ) ) );

        // 包含readAnyDatabase,clusterMonitor角色收集全部信息成功
        CommLib.collectALL( ssh, collectSample, user1, password, "admin" );
        ssh.exec( "cat " + collectorOutputPath + "/collection.json" );
        String[] split = ssh.getStdout().split( "\n" );
        ArrayList< String > collectionNames = new ArrayList<>();
        for ( String s : split ) {
            JSONObject jsonObject = JSONObject.parseObject( s );
            if ( jsonObject.getString( "database" ).equals( databaseName ) ) {
                Assert.assertEquals( jsonObject.getIntValue( "count" ), 100 );
                collectionNames.add( jsonObject.getString( "name" ) );
            }
        }
        Assert.assertTrue( collectionNames.contains( collectionName ) );
        ssh.exec( "cat " + collectorOutputPath + "/cluster.json" );
        ArrayList< String > databaseNames = new ArrayList<>();
        JSONArray databases = JSONObject.parseObject( ssh.getStdout() )
                .getJSONArray( "databases" );
        for ( Object jsonObject : databases ) {
            JSONObject databaseObject = ( JSONObject ) jsonObject;
            databaseNames.add( databaseObject.getString( "name" ) );
        }
        Assert.assertTrue( databaseNames.contains( databaseName ) );

        // 具有readAnyDatabase角色收集集合信息成功，收集集群信息，收集全部信息失败
        CommLib.collectCollection( ssh, collectSample, user2, password,
                "admin" );
        ssh.exec( "cat " + collectorOutputPath + "/collection.json" );
        split = ssh.getStdout().split( "\n" );
        collectionNames = new ArrayList<>();
        for ( String s : split ) {
            JSONObject jsonObject = JSONObject.parseObject( s );
            if ( jsonObject.getString( "database" ).equals( databaseName ) ) {
                Assert.assertEquals( jsonObject.getIntValue( "count" ), 100 );
                collectionNames.add( jsonObject.getString( "name" ) );
            }
        }
        Assert.assertTrue( collectionNames.contains( collectionName ) );
        try {
            CommLib.collectCluster( ssh, user2, password, "admin" );
            Assert.fail( "expected failed but success" );
        } catch ( Exception e ) {
            Assert.assertTrue( ssh.getStderr()
                    .contains( "not authorized on admin to execute command" ) );
        }
        try {
            CommLib.collectALL( ssh, collectSample, user2, password, "admin" );
            Assert.fail( "expected failed but success" );
        } catch ( Exception e ) {
            Assert.assertTrue( ssh.getStderr()
                    .contains( "not authorized on admin to execute command" ) );
        }

        // 具有clusterMonitor角色收集集群信息，收集集合信息，收集全部信息都失败
        try {
            CommLib.collectCollection( ssh, collectSample, user3, password,
                    "admin" );
            Assert.fail( "expected failed but success" );
        } catch ( Exception e ) {
            Assert.assertTrue( ssh.getStderr().contains( "not authorized on" ),
                    ssh.getStdout() );
        }
        // 需要有listCollections权限才可以成功收集集群信息
        try {
            CommLib.collectCluster( ssh, user3, password, "admin" );
            Assert.fail( "expected failed but success" );
        } catch ( Exception e ) {
            Assert.assertTrue( ssh.getStderr().contains( "not authorized on" ),
                    ssh.getStdout() );
        }
        try {
            CommLib.collectALL( ssh, collectSample, user3, password, "admin" );
            Assert.fail( "expected failed but success" );
        } catch ( Exception e ) {
            Assert.assertTrue( ssh.getStderr().contains( "not authorized on" ),
                    ssh.getStdout() );
        }

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( "admin" )
                .runCommand( new Document( "dropUser", user1 ) );
        mongoClient.getDatabase( "admin" )
                .runCommand( new Document( "dropUser", user2 ) );
        mongoClient.getDatabase( "admin" )
                .runCommand( new Document( "dropUser", user3 ) );
        mongoClient.getDatabase( databaseName ).drop();
        if ( mongoClient != null ) {
            mongoClient.close();
        }
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
