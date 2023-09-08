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
 * @Descreption seqDB-32714 :: 版本: 1 :: 全量同步过程中执行DDL操作
 * @Author wangxingming
 * @CreateDate 2023/8/10
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/10
 * @Version 1.0
 */
public class SequoiaShake32714 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "multiRecord.json";
    int workerNum = 4;
    String cs = "mgocs_1";
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

        // 1. 删除全量同步中的数据库
        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 判断源端mongodb部署模式
        final boolean flag = CommLib.checkMongoMode( CommLib.confFileAll, ssh,
                mongoClient );

        // 将数据迁移至目标端的同时，源端删除库
        Callable< Void > task1 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                CommLib.genSequoiaShakeBackground( CommLib.confFileAll, ssh );
                return null;
            }
        };
        Callable< Void > task2 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                if ( flag ) {
                    Thread.sleep( 1000 );
                    mongoClient.getDatabase( cs ).drop();
                }
                return null;
            }
        };
        ExecutorService executorService1 = Executors.newFixedThreadPool( 2 );
        List< Callable< Void > > tasks1 = Arrays.asList( task1, task2 );
        executorService1.invokeAll( tasks1 );
        executorService1.shutdown();

        // // 对比源端和目标端数据
        // CommLib.checkRecordCount(cs, cl, 0, 7168);
        //
        // CommLib.dropDatabase(cs);
        //
        // // 2. 删除全量同步中的集合
        // // 构造数据
        // CommLib.genDataToMongoDB(configFile, workerNum, ssh);
        //
        // // 将数据迁移至目标端的同时，源端删除库
        // Callable<Void> task3 = () -> {
        // CommLib.genSequoiaShakeBackground(CommLib.confFileAll, ssh);
        // return null;
        // };
        // Callable<Void> task4 = () -> {
        // Thread.sleep(1000);
        // mongoClient.getDatabase(cs).getCollection(cl).drop();
        // mongoClient.close();
        // return null;
        // };
        // ExecutorService executorService2 = Executors.newFixedThreadPool(2);
        // List<Callable<Void>> tasks2 = Arrays.asList(task3, task4);
        // executorService2.invokeAll(tasks2);
        // executorService2.shutdown();
        //
        //
        // CommLib.dropDatabase(cs);
        //
        // // 3. 删除全量同步中的索引
        // // 构造数据
        // CommLib.genDataToMongoDB(configFile, workerNum, ssh);
        //
        // // 将数据迁移至目标端的同时，源端删除库
        // Callable<Void> task5 = () -> {
        // CommLib.genSequoiaShakeBackground(CommLib.confFileAll, ssh);
        // return null;
        // };
        // Callable<Void> task6 = () -> {
        // Thread.sleep(1000);
        // mongoClient.getDatabase(cs).getCollection(cl).dropIndexes();
        // mongoClient.close();
        // return null;
        // };
        // ExecutorService executorService3 = Executors.newFixedThreadPool(2);
        // List<Callable<Void>> tasks3 = Arrays.asList(task5, task6);
        // executorService3.invokeAll(tasks3);
        // executorService3.shutdown();

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            CommLib.createSsh().execBackground(
                    "kill -9 `ps -ef | grep collector.linux_x86 | awk '{print $2}'`" );
            CommLib.dropDatabase( cs, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}
