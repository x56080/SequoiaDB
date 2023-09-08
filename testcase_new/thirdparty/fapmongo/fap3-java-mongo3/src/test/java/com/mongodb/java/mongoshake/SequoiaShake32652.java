package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * @Descreption seqDB-32652 :: 版本: 1 :: 并发执行全量同步
 * @Author wangxingming
 * @CreateDate 2023/8/5
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/5
 * @Version 1.0
 */
public class SequoiaShake32652 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "multiRecord.json";
    int workerNum = 4;
    String cs = "mgocs_1";

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
        Ssh ssh = CommLib.createSsh();

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 将数据迁移至目标端
        Callable< Void > task1 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                CommLib.genSequoiaShake( "collector_full.conf",
                        CommLib.createSsh() );
                return null;
            }
        };
        Callable< Void > task2 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                CommLib.genSequoiaShake( "collector_full_1.conf",
                        CommLib.createSsh() );
                return null;
            }
        };
        ExecutorService executorService = Executors.newFixedThreadPool( 2 );
        List< Callable< Void > > tasks = Arrays.asList( task1, task2 );
        executorService.invokeAll( tasks );
        executorService.shutdown();

        // 对比源端和目标端数据
        CommLib.checkDataResult( "all", "config,sequoiashake", ssh );

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.dropDatabase( cs, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}
