package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Arrays;

/**
 * @Descreption seqDB-33020 :: 版本: 1 :: 全量同步指定用户具有writeOnly权限
 * @Author wangxingming
 * @CreateDate 2023/8/25
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/25
 * @Version 1.0
 */
public class SequoiaShake33020 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String adminUser = "admin";
    String adminPasswd = "admin";
    String username = "user";
    String password = "user";
    String role = "writeOnly";
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

        // 自定义roles
        CommLib.createRole( mongoClient, role,
                Arrays.asList( "insert", "listCollections", "listIndexes" ) );

        // 创建具有writeOnly权限的用户
        CommLib.createUser( mongoClient, username, password, role );

        // 将数据迁移至目标端
        try {
            CommLib.genSequoiaShake( CommLib.confFileFull, ssh );
        } catch ( Exception e ) {
            if ( e.getMessage() == null ) {
                throw e;
            } else {
                System.out.println(
                        "---exe: The error is reported as expected\n" );
            }
        }

        // 对比源端和目标端数据
        CommLib.checkRecordNotEqual( cs1, cl1, mongoClient, sequoiaClient );
        CommLib.checkRecordNotEqual( cs1, cl2, mongoClient, sequoiaClient );
        CommLib.checkRecordNotEqual( cs2, cl1, mongoClient, sequoiaClient );
        CommLib.checkRecordNotEqual( cs2, cl2, mongoClient, sequoiaClient );

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.dropUser( mongoClient, username );
            CommLib.dropRole( mongoClient, role );
            CommLib.dropDatabase( cs1, mongoClient, sequoiaClient );
            CommLib.dropDatabase( cs2, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}
