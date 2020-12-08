
package com.mongodb.springdata.serial;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.springframework.data.mongodb.core.MongoTemplate;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.CommandResult;
import com.mongodb.MongoClient;
import com.mongodb.MongoClientOptions;
import com.mongodb.MongoCredential;
import com.mongodb.ServerAddress;
import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21934:增删查用户
 * @Author chimanzhao 2020/6/23 huangxiaoni 2020/12/5
 */

public class Authentication21934 extends MongodbTestBase {
    private MongoClientOptions opt = null;
    private MongoClient client;
    private MongoClient client1 = null;
    private MongoClient client2 = null;
    private MongoTemplate temp;
    private MongoTemplate temp1 = null;
    private MongoTemplate temp2 = null;
    private String username1 = "mongo_springdata_21934_1";
    private String username2 = "mongo_springdata_21934_2";
    private String dbName = "mongo_springdata_21934";
    private String clName = "cl21934";
    private int recordsNum = 6;
    private List< Entity > records;
    private boolean runSuccess = false;

    @BeforeClass
    public void setup() throws Exception {
        // 创建client
        opt = MongoClientOptions.builder().build();
        try {
            client = new MongoClient(
                    new ServerAddress( config.getHost(), config.getPort() ),
                    opt );
            temp = new MongoTemplate( client, dbName );

            // 鉴权跳过SDB独立模式
            CommandResult result = null;
            try {
                result = temp.executeCommand(
                        new BasicDBObject( "createUser", username1 )
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
                        MongoTemplate tmpTemp = new MongoTemplate(
                                tmpClient, dbName );
                        // 删除数据库
                        tmpClient.dropDatabase( dbName );
                        // 删除用户
                        tmpTemp.executeCommand(
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
                records.add( new Entity( "a" + i,
                        Entity.SEXS[ i % Entity.SEXS.length ], i, i,
                        Entity.COURSES ) );
            }
        } catch ( Exception e ) {
            if ( client != null )
                client.close();
            throw e;
        }
    }

    @Test
    public void test() throws UnknownHostException {
        // 创建用户
        // 创建多个用户
        temp.executeCommand( new BasicDBObject( "createUser", username1 )
                .append( "pwd", username1 )
                .append( "roles", Collections.singletonList( "dbOwner" ) ) );
        temp.executeCommand( new BasicDBObject( "createUser", username2 )
                .append( "pwd", username2 )
                .append( "roles", Collections.singletonList( "dbOwner" ) ) );
        // 重复创建用户
        temp.executeCommand( new BasicDBObject( "createUser", username2 )
                .append( "pwd", username2 )
                .append( "roles", Collections.singletonList( "dbOwner" ) ) );

        // 查询用户
        // 查询所有用户
        CommandResult userInfo1 = temp
                .executeCommand( new BasicDBObject( "usersInfo", 1 ) );
        Assert.assertEquals( userInfo1.get( "users" ),
                Arrays.asList( new BasicDBObject( "user", username1 ),
                        new BasicDBObject( "user", username2 ) ) );
        // 查询单个用户
        CommandResult userInfo2 = temp
                .executeCommand( new BasicDBObject( "usersInfo",
                        Collections.singletonList(
                                new BasicDBObject( "user", username1 )
                                        .append( "db", dbName ) ) ) );
        Assert.assertEquals( userInfo2.get( "users" ),
                Arrays.asList( new BasicDBObject( "user", username1 ) ) );

        // 鉴权
        // 用户1鉴权
        MongoCredential cred1 = MongoCredential.createScramSha1Credential(
                username1, dbName, username1.toCharArray() );
        client1 = new MongoClient(
                new ServerAddress( config.getHost(), config.getPort() ),
                Collections.singletonList( cred1 ), opt );
        temp1 = new MongoTemplate( client1, dbName );
        // 用户2鉴权
        MongoCredential cred2 = MongoCredential.createScramSha1Credential(
                username2, dbName, username2.toCharArray() );
        client2 = new MongoClient(
                new ServerAddress( config.getHost(), config.getPort() ),
                Collections.singletonList( cred2 ), opt );
        temp2 = new MongoTemplate( client2, dbName );

        // 使用不存在的用户创建连接
        MongoCredential cred3 = MongoCredential.createScramSha1Credential(
                "noexists", dbName, "noexists".toCharArray() );
        MongoClient client3 = new MongoClient(
                new ServerAddress( config.getHost(), config.getPort() ),
                Collections.singletonList( cred3 ), opt );
        MongoTemplate temp3 = new MongoTemplate( client3, dbName );
        try {
            temp3.insert(
                    new Entity( "a", Entity.SEXS[ 0 % Entity.SEXS.length ], 1,
                            1, Entity.COURSES ) );
            Assert.fail( "Expect fail but actual success!!!" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "Authentication failed" ) ) {
                throw e;
            }
        }

        // 业务操作
        // 无鉴权，创建集合
        temp.createCollection( clName );
        // 用户1，插入、查询记录
        temp1.insert( records, clName );
        List< Entity > actList = temp1.findAll( Entity.class, clName );
        Assert.assertEquals( actList, records );
        // 用户2，删除、查询记录、删除集合
        Criteria criteria = Criteria.where( "name" ).is( "a0" );
        Query query = new Query( criteria );
        Assert.assertEquals( temp2.count( query, clName ), 1 );
        temp2.remove( query, clName );
        Assert.assertEquals( temp2.count( query, clName ), 0 );
        temp2.dropCollection( clName );
        Assert.assertFalse( temp2.collectionExists( clName ) );

        runSuccess = true;
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        try {
            if ( runSuccess )
                client.dropDatabase( dbName );
        } finally {
            // 不管用例执行结果如何，均需要清理新创建的用户、连接
            // 清理用户，创建用户成功但创建db1连接失败或在此之前失败，需要先鉴权再删除用户
            if ( temp1 == null ) {
                try {
                    MongoCredential cred1 = MongoCredential
                            .createScramSha1Credential( username1, dbName,
                                    username1.toCharArray() );
                    client1 = new MongoClient(
                            new ServerAddress( config.getHost(),
                                    config.getPort() ),
                            Collections.singletonList( cred1 ), opt );
                    temp1 = new MongoTemplate( client1, dbName );
                } catch ( Exception e ) {
                    // 忽略错误
                    e.printStackTrace();
                }
            }
            try {
                temp1.executeCommand(
                        new BasicDBObject( "dropUser", username1 ) );
            } catch ( Exception e ) {
                // 忽略错误
                e.printStackTrace();
            }

            try {
                MongoCredential cred2 = MongoCredential
                        .createScramSha1Credential( username2, dbName,
                                username2.toCharArray() );
                client2 = new MongoClient(
                        new ServerAddress( config.getHost(), config.getPort() ),
                        Collections.singletonList( cred2 ), opt );
                temp2 = new MongoTemplate( client2, dbName );
            } catch ( Exception e ) {
                // 忽略错误
                e.printStackTrace();
            }
            try {
                temp2.executeCommand(
                        new BasicDBObject( "dropUser", username2 ) );
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
