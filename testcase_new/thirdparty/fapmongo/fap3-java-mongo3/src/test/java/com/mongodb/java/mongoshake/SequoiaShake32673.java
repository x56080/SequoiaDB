package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32673 :: 版本: 1 :: 增量同步指定集合数据
 * @Author wangxingming
 * @CreateDate 2023/8/7
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/7
 * @Version 1.0
 */
public class SequoiaShake32673 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "recordAndIndex.json";
    int workerNum = 4;
    String cs1 = "mgocs_1";
    String cs2 = "mgocs_2";
    String cl = "mgocl_2";

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
        ssh.exec(
                "sed -i 's/filter.namespace.white =.*/filter.namespace.white = "
                        + "mgocs_1.mgocl_1/g' " + CommLib.toolPath
                        + CommLib.confFileIncr );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 判断源端mongodb部署模式
        CommLib.checkMongoMode( CommLib.confFileIncr, ssh, mongoClient );

        // 将数据迁移至目标端
        CommLib.genSequoiaShakeBackground( CommLib.confFileIncr, ssh );

        // 对比源端和目标端数据
        CommLib.checkCLNotExist( cs1, cl, sequoiaClient );
        // 源端删除目标端不存在的集合，支持对比
        mongoClient.getDatabase( cs1 ).getCollection( cl ).drop();
        CommLib.checkDataResult( "all", "config,sequoiashake,mgocs_2", ssh );

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            CommLib.createSsh().exec(
                    "sed -i 's/filter.namespace.white = .*/filter.namespace.white =/g' "
                            + CommLib.toolPath + CommLib.confFileIncr );
            CommLib.dropDatabase( cs1, mongoClient, sequoiaClient );
            CommLib.dropDatabase( cs2, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}