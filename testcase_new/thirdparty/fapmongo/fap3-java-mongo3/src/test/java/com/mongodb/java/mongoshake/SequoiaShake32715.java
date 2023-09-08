package com.mongodb.java.mongoshake;

import com.mongodb.MongoNamespace;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.model.RenameCollectionOptions;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32715 :: 版本: 1 :: 开启DDL，增量同步过程中执行DDL操作
 * @Author wangxingming
 * @CreateDate 2023/8/10
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/10
 * @Version 1.0
 */
public class SequoiaShake32715 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "baseType.json";
    int workerNum = 4;
    String cs = "mgocs_basetype";
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

        // 修改conf文件
        CommLib.updateCheckpoint( CommLib.confFileAll, ssh );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 判断源端mongodb部署模式
        boolean flag = CommLib.checkMongoMode( CommLib.confFileAll, ssh,
                mongoClient );

        // 将数据迁移至目标端
        CommLib.genSequoiaShakeBackground1( CommLib.confFileAll, ssh );
        if ( flag ) {
            MongoCollection< Document > collection = mongoClient
                    .getDatabase( cs ).getCollection( cl );
            collection.renameCollection( new MongoNamespace( cs, cl + "_new" ),
                    new RenameCollectionOptions().dropTarget( true ) );
            mongoClient.getDatabase( cs ).getCollection( cl + "_new" )
                    .dropIndexes();
            mongoClient.getDatabase( cs ).getCollection( cl + "_new" )
                    .createIndex( new Document( "int32", 1 ) );
            collection.insertOne( new Document( "name", "XCZVC" ) );
        }

        // 判断增量同步是否完成
        CommLib.checkIncr( ssh );

        // 对比源端和目标端数据
        CommLib.checkIndex( cs, mongoClient, sequoiaClient );
        CommLib.checkDataResult( "all", "config,sequoiashake", ssh );

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            CommLib.createSsh().execBackground(
                    "kill -9 `ps -ef | grep collector.linux_x86 | awk '{print $2}'`" );
            CommLib.dropDatabase( cs, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}
