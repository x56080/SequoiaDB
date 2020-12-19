
package com.mongodb.java.serial;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.CommandResult;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.MongoClient;
import com.mongodb.MongoClientOptions;
import com.mongodb.MongoCommandException;
import com.mongodb.MongoCredential;
import com.mongodb.ServerAddress;
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
    private DB db = null;
    private DB db1 = null;
    private DB db2 = null;
    private String username1 = "mongo_java_21934_1";
    private String username2 = "mongo_java_21934_2";
    private String dbName = "mongo_java_21934";
    private String clName = "cl_21934";
    private int recordsNum = 30;
    private List< DBObject > records;
    private boolean runSuccess = false;

    @BeforeClass
    public void setup() throws Exception {
        // 创建Client
        opt = MongoClientOptions.builder().build();
        try {
            client = new MongoClient(
                    new ServerAddress( config.getHost(), config.getPort() ),
                    opt );
            db = client.getDB( dbName );

            // 鉴权跳过SDB独立模式
            CommandResult result = null;
            try {
                result = db
                        .command( new BasicDBObject( "createUser", username1 )
                                .append( "pwd", username1 )
                                .append( "roles", Collections
                                        .singletonList( "dbOwner" ) ) );
                // SDB独立模式，executeCommand失败没有抛异常，需要获取打印的异常码做判断并跳过独立模式
                if ( result.getException() != null ) {
                    if ( result.getException().getCode() == -159 ) {
                        throw new SkipException( "Skip SDB standalone mode!" );
                    } else {
                        throw new Exception( result.getException() );
                    }
                }
            } finally {
                if ( result.getException() == null ) {
                    // 鉴权
                    MongoCredential tmpCred = MongoCredential
                            .createScramSha1Credential( username1, dbName,
                                    username1.toCharArray() );
                    MongoClient tmpClient = null;
                    try {
                        tmpClient = new MongoClient(
                                new ServerAddress( config.getHost(),
                                        config.getPort() ),
                                Collections.singletonList( tmpCred ), opt );
                        // 删除数据库
                        tmpClient.dropDatabase( dbName );
                        // 删除用户
                        DB tmpDB = tmpClient.getDB( dbName );
                        tmpDB.command(
                                new BasicDBObject( "dropUser", username1 ) );
                    } finally {
                        if ( tmpClient != null )
                            tmpClient.close();
                    }
                }
            }

            // 准备数据
            records = new ArrayList<>();
            for ( int i = 0; i < recordsNum; i++ ) {
                records.add( new BasicDBObject( "a", i ).append( "b", "" + i )
                        .append( "c", Arrays.asList( i + 1, i + 2, i + 3 ) )
                        .append( "d", "test-" + i + "-" + i % 3 )
                        .append( "e", new BasicDBObject( "a", 1 ).append( "b",
                                1 ) ) );
            }
        } catch ( Exception e ) {
            if ( client != null )
                client.close();
            throw e;
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void test() throws UnknownHostException {
        // 创建用户
        // 创建多个用户
        db.command( new BasicDBObject( "createUser", username1 )
                .append( "pwd", username1 )
                .append( "roles", Collections.singletonList( "dbOwner" ) ) );
        db.command( new BasicDBObject( "createUser", username2 )
                .append( "pwd", username2 )
                .append( "roles", Collections.singletonList( "dbOwner" ) ) );
        // 重复创建用户
        try {
            db.command( new BasicDBObject( "createUser", username2 )
                    .append( "pwd", username2 ).append( "roles",
                            Collections.singletonList( "dbOwner" ) ) );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -295 ) {
                throw e;
            }
        }

        // 查询用户
        // 查询所有用户
        CommandResult userInfo1 = db
                .command( new BasicDBObject( "usersInfo", 1 ) );
        String users1 = userInfo1.get( "users" ).toString();
        Assert.assertTrue(
                users1.contains(
                        new BasicDBObject( "user", username1 ).toString() ),
                users1 );
        Assert.assertTrue(
                users1.contains(
                        new BasicDBObject( "user", username2 ).toString() ),
                users1 );
        // 查询单个用户
        CommandResult userInfo2 = db.command( new BasicDBObject( "usersInfo",
                Collections
                        .singletonList( new BasicDBObject( "user", username1 )
                                .append( "db", dbName ) ) ) );
        List< BasicDBObject > users2 = ( List< BasicDBObject > ) userInfo2
                .get( "users" );
        Assert.assertEquals( users2,
                Arrays.asList( new BasicDBObject( "user", username1 ) ) );

        // 鉴权
        // 用户1鉴权
        MongoCredential cred1 = MongoCredential.createScramSha1Credential(
                username1, dbName, username1.toCharArray() );
        client1 = new MongoClient(
                new ServerAddress( config.getHost(), config.getPort() ),
                Collections.singletonList( cred1 ), opt );
        db1 = client1.getDB( dbName );
        // 用户2鉴权
        MongoCredential cred2 = MongoCredential.createScramSha1Credential(
                username2, dbName, username2.toCharArray() );
        client2 = new MongoClient(
                new ServerAddress( config.getHost(), config.getPort() ),
                Collections.singletonList( cred2 ), opt );
        db2 = client2.getDB( dbName );

        // 业务操作
        // 无鉴权，创建集合
        db.createCollection( clName, new BasicDBObject() );
        // 用户1，插入、查询记录
        DBCollection cl1 = db1.getCollection( clName );
        cl1.insert( records );
        List< DBObject > actList = cl1.find().toArray();
        Assert.assertEquals( actList, records );
        // 用户2，删除、查询记录、删除集合
        DBCollection cl2 = db2.getCollection( clName );
        DBObject query = new BasicDBObject( "a", 0 );
        Assert.assertEquals( cl2.count( query ), 1 );
        cl2.remove( query );
        Assert.assertEquals( cl2.count( query ), 0 );
        cl2.drop();
        Assert.assertFalse( db2.collectionExists( clName ) );

        // 使用不存在的用户创建连接
        MongoCredential cred3 = MongoCredential.createScramSha1Credential(
                "noexists", dbName, "noexists".toCharArray() );
        MongoClient client3 = new MongoClient(
                new ServerAddress( config.getHost(), config.getPort() ),
                Collections.singletonList( cred3 ), opt );
        DB db3 = client3.getDB( dbName );
        DBCollection cl3 = db3.getCollection( clName );
        try {
            cl3.insert( new BasicDBObject( "a", 1 ) );
            Assert.fail( "Expect fail but actual success!!!" );
        } catch ( Exception e ) {
            if ( !( e.getMessage().contains( "Authentication failed" ) || e
                    .getMessage().contains( "Exception authenticating" ) ) ) {
                throw e;
            }
        }

        // 删除用户
        db1.command( new BasicDBObject( "dropUser", username1 ) );
        db2.command( new BasicDBObject( "dropUser", username2 ) );
        // 重复删除用户
        db1.command( new BasicDBObject( "dropUser", username1 ) );
        // 检查删除结果
        CommandResult userInfo3 = db1
                .command( new BasicDBObject( "usersInfo", 1 ) );
        List< BasicDBObject > users3 = ( List< BasicDBObject > ) userInfo3
                .get( "users" );
        Assert.assertEquals( users3, Arrays.asList() );

        runSuccess = true;
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        try {
            if ( runSuccess )
                db.dropDatabase();
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
                    db1 = client1.getDB( dbName );
                } catch ( UnknownHostException e ) {
                    // 忽略错误
                    e.printStackTrace();
                }
            }
            try {
                db1.command( new BasicDBObject( "dropUser", username1 ) );
            } catch ( Exception e ) {
                // 忽略错误
                e.printStackTrace();
            }

            if ( db2 == null ) {
                try {
                    MongoCredential mongoCred2 = MongoCredential
                            .createScramSha1Credential( username2, dbName,
                                    username2.toCharArray() );
                    client2 = new MongoClient(
                            new ServerAddress( config.getHost(),
                                    config.getPort() ),
                            Collections.singletonList( mongoCred2 ), opt );
                    db2 = client2.getDB( dbName );
                } catch ( UnknownHostException e ) {
                    // 忽略错误
                    e.printStackTrace();
                }
            }
            try {
                db2.command( new BasicDBObject( "dropUser", username2 ) );
            } catch ( Exception e ) {
                // 忽略错误
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
