package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.time.Duration;
import java.time.Instant;

/**
 * @Descreption seqDB-32740 :: 版本: 1 :: 全量同步性能验证
 * @Author wangxingming
 * @CreateDate 2023/8/11
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/11
 * @Version 1.0
 */
public class SequoiaShake32740 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "testPerf.json";
    int workerNum = 4;
    String cs = "mgocs_1";
    String mongoDBHost = "192.168.31.6";
    String sequoiaDBHost = "192.168.31.6";
    int mongoDBPort = 27017;
    int sequoiaDBPort = 11817;

    @BeforeClass
    public void setUp() {
        CommLib.dropDatabaseDes( CommLib.MongoDBHost, CommLib.MongoDBPort, cs );
        CommLib.dropDatabaseDes( mongoDBHost, mongoDBPort, cs );
        CommLib.dropDatabaseDes( sequoiaDBHost, sequoiaDBPort, cs );
    }

    @Test
    public void test() throws Exception {
        // 创建SSH连接
        Ssh ssh = CommLib.createSsh();

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 将数据迁移至目标端
        ssh.exec( "sed -i 's#tunnel.address =.*#tunnel.address = mongodb://"
                + mongoDBHost + ":" + mongoDBPort + "#g' " + CommLib.toolPath
                + CommLib.confFileFull );
        Instant startTime1 = Instant.now();
        CommLib.genSequoiaShake( CommLib.confFileFull, ssh );
        Instant endTime1 = Instant.now();
        Duration duration1 = Duration.between( startTime1, endTime1 );
        System.out.println( "数据迁移至 mongodb 花费时间: " + duration1.toMinutes()
                + " 分钟 " + duration1.getSeconds() % 60 + " 秒" );

        ssh.exec( "sed -i 's#tunnel.address =.*#tunnel.address = mongodb://"
                + sequoiaDBHost + ":" + sequoiaDBPort + "#g' "
                + CommLib.toolPath + CommLib.confFileFull );
        Instant startTime2 = Instant.now();
        CommLib.genSequoiaShake( CommLib.confFileFull, ssh );
        Instant endTime2 = Instant.now();
        Duration duration2 = Duration.between( startTime2, endTime2 );
        System.out.println( "数据迁移至 sequoiadb 花费时间: " + duration2.toMinutes()
                + " 分钟 " + duration2.getSeconds() % 60 + " 秒" );

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() {
        CommLib.dropDatabaseDes( CommLib.MongoDBHost, CommLib.MongoDBPort, cs );
        CommLib.dropDatabaseDes( mongoDBHost, mongoDBPort, cs );
        CommLib.dropDatabaseDes( sequoiaDBHost, sequoiaDBPort, cs );
    }
}