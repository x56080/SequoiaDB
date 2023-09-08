package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.time.Duration;
import java.time.Instant;

/**
 * @Descreption seqDB-32733 :: 版本: 1 ::
 *              设置full_sync.reader.collection_parallel执行同步
 * @Author wangxingming
 * @CreateDate 2023/8/11
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/11
 * @Version 1.0
 */
public class SequoiaShake32733 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "testPerf.json";
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
        ssh.exec(
                "sed -i 's/full_sync.reader.collection_parallel =.*/full_sync.reader.collection_parallel = "
                        + "1/g' " + CommLib.toolPath + CommLib.confFileFull );
        Instant startTime1 = Instant.now();
        CommLib.genSequoiaShake( CommLib.confFileFull, ssh );
        Instant endTime1 = Instant.now();
        Duration duration1 = Duration.between( startTime1, endTime1 );
        System.out
                .println( "full_sync.reader.collection_parallel = 1 数据迁移花费时间: "
                        + duration1.toMinutes() + " 分钟 "
                        + duration1.getSeconds() % 60 + " 秒" );

        // 清理环境
        CommLib.dropDatabaseDes( CommLib.SequoiaDBHost,
                CommLib.SequoiaDBFapPort, cs );

        ssh.exec(
                "sed -i 's/full_sync.reader.collection_parallel =.*/full_sync.reader.collection_parallel = "
                        + "6/g' " + CommLib.toolPath + CommLib.confFileFull );
        Instant startTime2 = Instant.now();
        CommLib.genSequoiaShake( CommLib.confFileFull, ssh );
        Instant endTime2 = Instant.now();
        Duration duration2 = Duration.between( startTime2, endTime2 );
        System.out
                .println( "full_sync.reader.collection_parallel = 6 数据迁移花费时间: "
                        + duration2.toMinutes() + " 分钟 "
                        + duration2.getSeconds() % 60 + " 秒" );

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
