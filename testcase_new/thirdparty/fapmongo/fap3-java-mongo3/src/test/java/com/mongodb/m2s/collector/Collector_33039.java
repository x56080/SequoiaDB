package com.mongodb.m2s.collector;

import java.util.ArrayList;
import java.util.Arrays;

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
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-33039:使用不同权限角色收集集群信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/9/4
 * @UpdateRemark
 * @Version
 */
public class Collector_33039 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_33039";
    private String collectionName = "col_33039";
    private String adminUser = "user1_33039";
    private String testUser = "user2_33039";
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
        adminDatabase.runCommand( new Document( "createUser", adminUser )
                .append( "pwd", password ).append( "roles", Arrays
                        .asList( "readAnyDatabase", "clusterMonitor" ) ) );
        // 测试数据库下创建用户，包含dbOwner,userAdmin角色
        database.runCommand( new Document( "createUser", testUser )
                .append( "pwd", password )
                .append( "roles", Arrays.asList( "dbOwner", "userAdmin" ) ) );

        // 使用admin用户收集集群，集合信息,预期收集成功
        CommLib.collectCollection( ssh, collectSample, adminUser, password,
                "admin" );
        CommLib.collectCluster( ssh, adminUser, password, "admin" );
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

        // 使用test用户收集集合信息，预期收集成功，只收集得到用户所在的数据库下的集合信息
        CommLib.collectCollection( ssh, collectSample, testUser, password,
                databaseName );
        ssh.exec( "cat " + collectorOutputPath + "/collection.json" );
        split = ssh.getStdout().split( "\n" );
        Assert.assertEquals( split.length, 1 );
        JSONObject jsonObject = JSONObject.parseObject( split[ 0 ] );
        Assert.assertTrue(
                jsonObject.getString( "database" ).equals( databaseName ) );
        Assert.assertEquals( jsonObject.getIntValue( "count" ), 100 );
        Assert.assertTrue(
                jsonObject.getString( "name" ).equals( collectionName ) );

        // 使用test用户收集集群信息，预期收集失败
        try {
            CommLib.collectCluster( ssh, testUser, password, databaseName );
            Assert.fail( "test user should not collect cluster info" );
        } catch ( Exception e ) {
            Assert.assertTrue( ssh.getStderr()
                    .contains( "not authorized on admin to execute command" ) );
        }

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName )
                .runCommand( new Document( "dropUser", testUser ) );
        mongoClient.getDatabase( "admin" )
                .runCommand( new Document( "dropUser", adminUser ) );
        mongoClient.getDatabase( databaseName ).drop();
        if ( mongoClient != null ) {
            mongoClient.close();
        }
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }

}
