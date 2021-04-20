
package com.mongodb.java.serial;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.bson.Document;
import org.bson.conversions.Bson;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.MongoClient;
import com.mongodb.MongoClientOptions;
import com.mongodb.MongoCommandException;
import com.mongodb.MongoCredential;
import com.mongodb.ServerAddress;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Filters;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21934:增删用户
 * @Author chimanzao 2020/6/23 huangxiaoni 2020/12/5
 */

public class Authentication21934 extends MongodbTestBase {
    private MongoClientOptions opt = null;
    private MongoClient client = null;
    private MongoClient client1 = null;
    private MongoClient client2 = null;
    private MongoDatabase db = null;
    private MongoDatabase db1 = null;
    private MongoDatabase db2 = null;
    private String username1;
    private String username2;
    private String dbName;
    private String clName;
    private int recordsNum = 30;
    private List< Document > records;
    private boolean runSuccess = false;

    @SuppressWarnings("deprecation")
    @BeforeClass
    public void setup() throws Exception {
        username1 = javaDBNameWithVersion + "_user21934_1";
        username2 = javaDBNameWithVersion + "_user21934_2";
        dbName = javaDBNameWithVersion + "_db21934";
        clName = javaDBNameWithVersion + "_cl21934";

        // 创建Client
        opt = MongoClientOptions.builder().build();
        try {
            client = new MongoClient(
                    new ServerAddress( config.getHost(), config.getPort() ),
                    opt );
            db = client.getDatabase( dbName );

            // 鉴权跳过SDB独立模式
            try {
                db.runCommand( new Document( "createUser", username1 )
                        .append( "pwd", username1 ).append( "roles",
                                Collections.singletonList( "dbOwner" ) ) );
            } catch ( MongoCommandException e ) {
                if ( e.getCode() == -159 ) {
                    throw new SkipException( "Skip SDB standalone mode!" );
                }
            }

            client.dropDatabase( dbName );

            // 准备数据
            records = new ArrayList<>();
            for ( int i = 0; i < recordsNum; i++ ) {
                records.add( new Document( "a", i ).append( "b", "" + i )
                        .append( "c", Arrays.asList( i + 1, i + 2, i + 3 ) )
                        .append( "d", "test-" + i + "-" + i % 3 ).append( "e",
                                new Document( "a", 1 ).append( "b", 1 ) ) );
            }
        } catch ( Exception e ) {
            // 鉴权
            MongoCredential tmpCred = MongoCredential.createScramSha1Credential(
                    username1, dbName, username1.toCharArray() );
            MongoClient tmpClient = null;
            try {
                tmpClient = new MongoClient(
                        new ServerAddress( config.getHost(), config.getPort() ),
                        Collections.singletonList( tmpCred ), opt );
                // 删除数据库
                tmpClient.dropDatabase( dbName );
                // 删除用户
                MongoDatabase tmpDB = tmpClient.getDatabase( dbName );
                tmpDB.runCommand( new BasicDBObject( "dropUser", username1 ) );
            } finally {
                if ( tmpClient != null )
                    tmpClient.close();
            }

            if ( client != null )
                client.close();
            throw e;
        }
    }

    @SuppressWarnings({ "unchecked", "resource", "deprecation" })
    @Test
    public void test() throws UnknownHostException {
        // 创建用户
        // 创建多个用户
        // 用户1在setUp已创建
        db.runCommand( new BasicDBObject( "createUser", username2 )
                .append( "pwd", username2 )
                .append( "roles", Collections.singletonList( "dbOwner" ) ) );
        // 重复创建用户
        try {
            db.runCommand( new BasicDBObject( "createUser", username2 )
                    .append( "pwd", username2 ).append( "roles",
                            Collections.singletonList( "dbOwner" ) ) );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -295 ) {
                throw e;
            }
        }

        // 查询用户
        // 查询所有用户
        Document userInfo1 = db.runCommand( new Document( "usersInfo", 1 ) );
        String users1 = userInfo1.get( "users" ).toString();
        Assert.assertTrue(
                users1.contains( new Document( "user", username1 ).toString() ),
                users1 );
        Assert.assertTrue(
                users1.contains( new Document( "user", username2 ).toString() ),
                users1 );
        // 查询单个用户
        Document userInfo2 = db.runCommand( new Document( "usersInfo",
                Collections.singletonList( new Document( "user", username1 )
                        .append( "db", dbName ) ) ) );
        Assert.assertEquals( userInfo2.get( "users", ArrayList.class ),
                Arrays.asList( new Document( "user", username1 ) ) );

        // 鉴权
        // 用户1鉴权
        MongoCredential cred1 = MongoCredential.createScramSha1Credential(
                username1, dbName, username1.toCharArray() );
        client1 = new MongoClient(
                new ServerAddress( config.getHost(), config.getPort() ),
                Collections.singletonList( cred1 ), opt );
        db1 = client1.getDatabase( dbName );
        // 用户2鉴权
        MongoCredential cred2 = MongoCredential.createScramSha1Credential(
                username2, dbName, username2.toCharArray() );
        client2 = new MongoClient(
                new ServerAddress( config.getHost(), config.getPort() ),
                Collections.singletonList( cred2 ), opt );
        db2 = client2.getDatabase( dbName );

        // 业务操作
        // 无鉴权，创建集合
        db.createCollection( clName );
        // 用户1，插入、查询记录
        MongoCollection< Document > cl1 = db1.getCollection( clName );
        cl1.insertMany( records );
        List< Document > actList = cl1.find()
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actList, records );
        // 用户2，删除、查询记录、删除集合
        MongoCollection< Document > cl2 = db2.getCollection( clName );
        Bson query = Filters.eq( "a", 0 );
        Assert.assertEquals( cl2.count( query ), 1 );
        cl2.deleteMany( query );
        Assert.assertEquals( cl2.count( query ), 0 );
        cl2.drop();
        try {
            cl2.count( query );
        } catch ( MongoCommandException e ) {
            if ( e.getCode() != -23 ) {
                throw e;
            }
        }

        // 使用不存在的用户创建连接
        MongoCredential cred3 = MongoCredential.createScramSha1Credential(
                "noexists", dbName, "noexists".toCharArray() );
        MongoClient client3 = new MongoClient(
                new ServerAddress( config.getHost(), config.getPort() ),
                Collections.singletonList( cred3 ), opt );
        MongoDatabase db3 = client3.getDatabase( dbName );
        MongoCollection< Document > cl3 = db3.getCollection( clName );
        try {
            cl3.insertOne( new Document( "a", 1 ) );
            Assert.fail( "Expect fail but actual success!!!" );
        } catch ( Exception e ) {
            if ( !( e.getMessage().contains( "Authentication failed" ) || e
                    .getMessage().contains( "Exception authenticating" ) ) ) {
                throw e;
            }
        }

        // 删除用户
        db1.runCommand( new BasicDBObject( "dropUser", username1 ) );
        db2.runCommand( new BasicDBObject( "dropUser", username2 ) );
        // 重复删除用户
        try {
            db1.runCommand( new BasicDBObject( "dropUser", username1 ) );
        } catch ( MongoCommandException e ) {
            if ( e.getCode() != -300 ) {
                throw e;
            }
        }
        // 检查删除结果
        Document userInfo3 = db1
                .runCommand( new BasicDBObject( "usersInfo", 1 ) );
        List< BasicDBObject > users3 = ( List< BasicDBObject > ) userInfo3
                .get( "users" );
        Assert.assertEquals( users3, Arrays.asList() );

        runSuccess = true;
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown( ITestContext context ) {
        try {
            if ( runSuccess )
                client.dropDatabase( dbName );
        } finally {
            // 不管用例执行结果如何，均需要清理新创建的用户、连接
            // 清理用户，创建用户成功但创建db1连接失败或在此之前失败，需要先鉴权再删除用户
            if ( db1 == null ) {
                try {
                    MongoCredential mongoCred1 = MongoCredential
                            .createScramSha1Credential( username1, dbName,
                                    username1.toCharArray() );
                    client1 = new MongoClient(
                            new ServerAddress( config.getHost(),
                                    config.getPort() ),
                            Collections.singletonList( mongoCred1 ), opt );
                    db1 = client1.getDatabase( dbName );
                } catch ( Exception e ) {
                    // 忽略异常
                    e.printStackTrace();
                }
            }
            try {
                db1.runCommand( new BasicDBObject( "dropUser", username1 ) );
            } catch ( Exception e ) {
                // 忽略异常
                e.printStackTrace();
            }

            if ( db2 == null ) {
                try {
                    MongoCredential mongoCred1 = MongoCredential
                            .createScramSha1Credential( username2, dbName,
                                    username2.toCharArray() );
                    client2 = new MongoClient(
                            new ServerAddress( config.getHost(),
                                    config.getPort() ),
                            Collections.singletonList( mongoCred1 ), opt );
                    db1 = client2.getDatabase( dbName );
                } catch ( Exception e ) {
                    // 忽略异常
                    e.printStackTrace();
                }
            }
            try {
                db2.runCommand( new BasicDBObject( "dropUser", username2 ) );
            } catch ( Exception e ) {
                // 忽略异常
                e.printStackTrace();
            }

            // 清理连接
            if ( client1 != null )
                client1.close();
            if ( client2 != null )
                client2.close();
            if ( client != null )
                client.close();
        }
    }
}
