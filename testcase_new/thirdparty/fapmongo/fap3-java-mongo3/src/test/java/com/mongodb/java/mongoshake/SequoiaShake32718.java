package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.model.Filters;
import com.mongodb.client.model.Updates;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * @Descreption seqDB-32718 :: 版本: 1 :: 同步过程中并发更新/删除相同数据
 * @Author wangxingming
 * @CreateDate 2023/8/10
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/10
 * @Version 1.0
 */
public class SequoiaShake32718 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "baseType.json";
    int workerNum = 4;
    String cs = "mgocs_basetype";
    String cl = "mgocl";

    @BeforeClass
    public void setUp() {
        mongoClient = MongoClients.create( "mongodb://" + CommLib.MongoDBHost
                + ":" + CommLib.MongoDBPort );
        sequoiaClient = MongoClients.create( "mongodb://"
                + CommLib.SequoiaDBHost + ":" + CommLib.SequoiaDBFapPort );
        CommLib.dropDatabase( cs, mongoClient, sequoiaClient );
    }

    @Test
    public void test() throws Exception {
        // 创建SSH连接
        final Ssh ssh = CommLib.createSsh();
        final MongoCollection< Document > collection = mongoClient
                .getDatabase( cs ).getCollection( cl );

        // 修改conf文件
        CommLib.updateCheckpoint( CommLib.confFileAll, ssh );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );
        for ( int i = 1; i <= 200; i++ ) {
            collection.insertOne(
                    new Document( "name", "ZXCV" + i ).append( "id", i ) );
        }

        // 判断源端mongodb部署模式
        CommLib.checkMongoMode( CommLib.confFileAll, ssh, mongoClient );

        // 将数据迁移至目标端
        Callable< Void > task1 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                for ( int i = 101; i <= 200; i++ ) {
                    collection.deleteOne( Filters.eq( "id", i ) );
                }
                return null;
            }
        };
        Callable< Void > task2 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                for ( int i = 1; i <= 100; i++ ) {
                    collection.updateOne( Filters.eq( "id", i ),
                            Updates.set( "name", "ASDF" + i ) );
                }
                return null;
            }
        };
        Callable< Void > task3 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                CommLib.genSequoiaShakeBackground( CommLib.confFileAll, ssh );
                return null;
            }
        };
        ExecutorService executorService = Executors.newFixedThreadPool( 3 );
        List< Callable< Void > > tasks = Arrays.asList( task1, task2, task3 );
        executorService.invokeAll( tasks );
        executorService.shutdown();

        // 对比源端和目标端数据
        CommLib.checkDataResult( "all", "config,sequoiashake", ssh );

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            CommLib.dropDatabase( cs, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}