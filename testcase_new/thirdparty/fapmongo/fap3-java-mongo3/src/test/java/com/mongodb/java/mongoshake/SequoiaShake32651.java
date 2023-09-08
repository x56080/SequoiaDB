package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
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
 * @Descreption seqDB-32651 :: 版本: 1 :: 删除非全量同步的集合
 * @Author wangxingming
 * @CreateDate 2023/8/5
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/5
 * @Version 1.0
 */
public class SequoiaShake32651 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "multiRecord.json";
    int workerNum = 4;
    String cs = "mgocs_1";
    String cl = "mgocl_1";

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

        // 修改conf文件
        ssh.exec(
                "sed -i 's/filter.namespace.white =.*/filter.namespace.white = "
                        + "mgocs_1.mgocl/g' " + CommLib.toolPath
                        + CommLib.confFileFull );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 将数据迁移至目标端的同时，删除非全量同步的集合
        Callable< Void > task1 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                CommLib.genSequoiaShake( CommLib.confFileFull, ssh );
                return null;
            }
        };
        Callable< Void > task2 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                Thread.sleep( 100 );
                mongoClient.getDatabase( cs ).getCollection( cl ).drop();
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
            CommLib.createSsh().exec(
                    "sed -i 's/filter.namespace.white =.*/filter.namespace.white = /g' "
                            + CommLib.toolPath + CommLib.confFileFull );
            CommLib.dropDatabase( cs, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}
