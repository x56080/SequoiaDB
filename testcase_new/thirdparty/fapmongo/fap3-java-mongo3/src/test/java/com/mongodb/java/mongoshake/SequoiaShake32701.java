package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32701 :: 版本: 1 :: 增量同步过程中创建集合为指定黑名单集合
 * @Author wangxingming
 * @CreateDate 2023/8/8
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/8
 * @Version 1.0
 */
public class SequoiaShake32701 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "baseType.json";
    int workerNum = 4;
    String cs = "mgocs_basetype";
    String cl_new = "mgocl_new";

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

        // 获取当前checkpoint并修改conf文件
        CommLib.updateCheckpoint( CommLib.confFileIncr, ssh );
        ssh.exec(
                "sed -i 's/filter.namespace.black =.*/filter.namespace.black = "
                        + "mgocs_basetype.mgocl_new/g' " + CommLib.toolPath
                        + CommLib.confFileIncr );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 判断源端mongodb部署模式
        boolean flag = CommLib.checkMongoMode( CommLib.confFileIncr, ssh,
                mongoClient );

        // 将数据迁移至目标端
        CommLib.genSequoiaShakeBackground1( CommLib.confFileIncr, ssh );

        // 判断增量同步是否完成
        // 对比源端和目标端数据
        if ( flag ) {
            mongoClient.getDatabase( cs ).getCollection( cl_new )
                    .insertOne( new Document( "name", "XCZVC" ) );
            CommLib.checkIncr( ssh );
            CommLib.checkCLNotExist( cs, cl_new, sequoiaClient );
        }

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            CommLib.createSsh().execBackground(
                    "kill -9 `ps -ef | grep collector.linux_x86 | awk '{print $2}'`" );
            CommLib.createSsh().exec(
                    "sed -i 's/filter.namespace.black =.*/filter.namespace.black =/g' "
                            + CommLib.toolPath + CommLib.confFileIncr );
            CommLib.dropDatabase( cs, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}