package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32627 :: 版本: 1 :: 全量同步忽略指定的集合
 * @Author wangxingming
 * @CreateDate 2023/8/3
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/3
 * @Version 1.0
 */
public class SequoiaShake32627 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "recordAndIndex.json";
    int workerNum = 4;
    String cs1 = "mgocs_1";
    String cs2 = "mgocs_2";
    String cl1 = "mgocl_1";
    String cl2 = "mgocl_2";

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

        // 修改conf文件
        ssh.exec(
                "sed -i 's/filter.namespace.black =.*/filter.namespace.black = "
                        + "mgocs_1.mgocl_1;mgocs_2.mgocl_2/g' "
                        + CommLib.toolPath + CommLib.confFileFull );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 将数据迁移至目标端
        CommLib.genSequoiaShake( CommLib.confFileFull, ssh );

        // 对比源端和目标端数据
        CommLib.checkCLNotExist( cs1, cl1, sequoiaClient );
        CommLib.checkCLNotExist( cs2, cl2, sequoiaClient );
        // 源端删除目标端不存在的集合，支持对比
        mongoClient.getDatabase( cs1 ).getCollection( cl1 ).drop();
        mongoClient.getDatabase( cs2 ).getCollection( cl2 ).drop();
        CommLib.checkDataResult( "all", "config,sequoiashake", ssh );

        // 关闭SSH连接
        ssh.disconnect();
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
