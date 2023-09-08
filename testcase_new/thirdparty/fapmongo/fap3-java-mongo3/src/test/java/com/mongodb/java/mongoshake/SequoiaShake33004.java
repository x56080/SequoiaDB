package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33004 :: 版本: 1 :: 全量同步指定用户具有readWriteAnyDatabase权限
 * @Author wangxingming
 * @CreateDate 2023/8/25
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/25
 * @Version 1.0
 */
public class SequoiaShake33004 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String adminUser = "admin";
    String adminPasswd = "admin";
    String username = "user";
    String password = "user";
    String configFile = "recordAndIndex.json";
    int workerNum = 4;
    String cs1 = "mgocs_1";
    String cs2 = "mgocs_2";
    String cl1 = "mgocl_1";
    String cl2 = "mgocl_2";

    @BeforeClass
    public void setUp() {
        mongoClient = MongoClients
                .create( "mongodb://" + adminUser + ":" + adminPasswd + "@"
                        + CommLib.MongoDBHost + ":" + CommLib.MongoDBPort );
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
        ssh.exec( "sed -i 's#mongo_urls =.*#mongo_urls = " + "mongodb://"
                + username + ":" + password + "@" + CommLib.MongoDBHost + ":"
                + CommLib.MongoDBPort + "#g' " + CommLib.toolPath + CommLib.confFileFull );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 创建具有readWriteAnyDatabase权限的用户
        CommLib.createUser( mongoClient, username, password,
                "readWriteAnyDatabase" );

        // 将数据迁移至目标端
        CommLib.genSequoiaShake( CommLib.confFileFull, ssh );

        // 对比源端和目标端数据
        CommLib.checkRecordEqual( cs1, cl1, mongoClient, sequoiaClient );
        CommLib.checkRecordEqual( cs1, cl2, mongoClient, sequoiaClient );
        CommLib.checkRecordEqual( cs2, cl1, mongoClient, sequoiaClient );
        CommLib.checkRecordEqual( cs2, cl2, mongoClient, sequoiaClient );

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.dropUser( mongoClient, username );
            CommLib.dropDatabase( cs1, mongoClient, sequoiaClient );
            CommLib.dropDatabase( cs2, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}