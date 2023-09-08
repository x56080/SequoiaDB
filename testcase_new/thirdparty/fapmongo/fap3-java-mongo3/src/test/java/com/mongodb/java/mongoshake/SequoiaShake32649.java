package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.model.Filters;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32649 :: 版本: 1 :: 全量同步完成后删除数据
 * @Author wangxingming
 * @CreateDate 2023/8/4
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/4
 * @Version 1.0
 */
public class SequoiaShake32649 {
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
        Ssh ssh = CommLib.createSsh();

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 将数据迁移至目标端的同时，源端删除数据
        CommLib.genSequoiaShake( CommLib.confFileFull, ssh );
        mongoClient.getDatabase( cs ).getCollection( cl )
                .deleteOne( Filters.eq( "int32", 20 ) );

        // 对比源端和目标端数据
        CommLib.checkRecordNotEqual( cs, cl, mongoClient, sequoiaClient );

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
