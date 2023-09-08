package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32626 :: 版本: 1 :: 全量同步忽略指定的数据库
 * @Author wangxingming
 * @CreateDate 2023/8/3
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/3
 * @Version 1.0
 */
public class SequoiaShake32626 {
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
        try {
            // 创建SSH连接
            Ssh ssh = CommLib.createSsh();

            // 修改conf文件
            ssh.exec(
                    "sed -i 's/filter.namespace.black =.*/filter.namespace.black = mgocs_1/g' "
                            + CommLib.toolPath + CommLib.confFileFull );

            // 构造数据
            CommLib.genDataToMongoDB( configFile, workerNum, ssh );

            // 将数据迁移至目标端
            CommLib.genSequoiaShake( CommLib.confFileFull, ssh );

            // 对比源端和目标端数据
            CommLib.checkCSNotExist( cs1, sequoiaClient );
            CommLib.checkDataResult( "all", "config,sequoiashake,mgocs_1",
                    ssh );

            // 关闭SSH连接
            ssh.disconnect();
        } catch ( Exception e ) {
            throw e;
        }
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            CommLib.createSsh().exec(
                    "sed -i 's/filter.namespace.black =.*/filter.namespace.black = /g' "
                            + CommLib.toolPath + CommLib.confFileFull );
            CommLib.dropDatabase( cs1, mongoClient, sequoiaClient );
            CommLib.dropDatabase( cs2, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}