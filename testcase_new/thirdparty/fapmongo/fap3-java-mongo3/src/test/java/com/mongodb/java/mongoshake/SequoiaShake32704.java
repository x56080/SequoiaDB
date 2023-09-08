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
 * @Descreption seqDB-32704 :: 版本: 1 :: 并发执行增量同步
 * @Author wangxingming
 * @CreateDate 2023/8/9
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/9
 * @Version 1.0
 */
public class SequoiaShake32704 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "recordAndIndex.json";
    int workerNum = 4;
    String cs1 = "mgocs_1";
    String cs2 = "mgocs_2";

    @BeforeClass
    public void setUp() {
        mongoClient = MongoClients.create( "mongodb://" + CommLib.MongoDBHost
                + ":" + CommLib.MongoDBPort );
        sequoiaClient = MongoClients.create( "mongodb://"
                + CommLib.SequoiaDBHost + ":" + CommLib.SequoiaDBFapPort );
        CommLib.dropDatabase( cs1, mongoClient, sequoiaClient );
        CommLib.dropDatabase( cs2, mongoClient, sequoiaClient );
    }

    @Test
    public void test() throws Exception {
        // 创建SSH连接
        Ssh ssh = CommLib.createSsh();

        // 获取当前checkpoint并修改conf文件
        CommLib.updateCheckpoint( CommLib.confFileIncr, ssh );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 判断源端mongodb部署模式
        CommLib.checkMongoMode( CommLib.confFileIncr, ssh, mongoClient );

        // 将数据迁移至目标端
        Callable< Void > task1 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                CommLib.genSequoiaShakeBackground1( CommLib.confFileIncr,
                        CommLib.createSsh() );
                return null;
            }
        };
        Callable< Void > task2 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                CommLib.genSequoiaShakeBackground1( CommLib.confFileIncr,
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
    public void tearDown() throws Exception {
        try {
            CommLib.createSsh().execBackground(
                    "kill -9 `ps -ef | grep collector.linux_x86 | awk '{print $2}'`" );
            CommLib.dropDatabase( cs1, mongoClient, sequoiaClient );
            CommLib.dropDatabase( cs2, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}
