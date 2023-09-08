package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32669 :: 版本: 1 :: 增量同步所有数据
 * @Author wangxingming
 * @CreateDate 2023/8/7
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/7
 * @Version 1.0
 */
public class SequoiaShake32669 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "recordAndIndex.json";
    int workerNum = 4;
    String cs1 = "mgocs_1";
    String cs2 = "mgocs_2";
    String cl1 = "mgocl_1";
    String cl2 = "mgocl_2";

    @BeforeClass
    public void setUp() throws Exception {
        mongoClient = MongoClients.create( "mongodb://" + CommLib.MongoDBHost
                + ":" + CommLib.MongoDBPort );
        sequoiaClient = MongoClients.create( "mongodb://"
                + CommLib.SequoiaDBHost + ":" + CommLib.SequoiaDBFapPort );
        CommLib.dropDatabase( cs1, mongoClient, sequoiaClient );
        CommLib.dropDatabase( cs2, mongoClient, sequoiaClient );

        // 目标端提前创建集合空间/集合
        // MongoClient mongoClient = MongoClients.create("mongodb://" +
        // CommLib.SequoiaDBHost + ":" + CommLib.SequoiaDBFapPort);
        // mongoClient.getDatabase(cs1).createCollection(cl1);
        // mongoClient.getDatabase(cs1).createCollection(cl2);
        // mongoClient.getDatabase(cs2).createCollection(cl1);
        // mongoClient.getDatabase(cs2).createCollection(cl2);
        // mongoClient.close();
    }

    @Test
    public void test() throws Exception {
        // 创建SSH连接
        Ssh ssh = CommLib.createSsh();

        // 修改conf文件
        CommLib.updateCheckpoint( CommLib.confFileIncr, ssh );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 判断源端mongodb部署模式
        CommLib.checkMongoMode( CommLib.confFileIncr, ssh, mongoClient );

        // 将数据迁移至目标端
        CommLib.genSequoiaShakeBackground( CommLib.confFileIncr, ssh );

        // 对比源端和目标端数据
        CommLib.checkDataResult( "all", "config,sequoiashake", ssh );

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            CommLib.dropDatabase( cs1, mongoClient, sequoiaClient );
            CommLib.dropDatabase( cs2, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}