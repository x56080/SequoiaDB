package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.model.Filters;
import com.mongodb.client.model.UpdateOptions;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32699 :: 版本: 1 :: 更新不在范围内数据，执行增量同步
 * @Author wangxingming
 * @CreateDate 2023/8/8
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/8
 * @Version 1.0
 */
public class SequoiaShake32699 {
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

        // 获取当前checkpoint并修改conf文件
        CommLib.updateCheckpoint( CommLib.confFileIncr, ssh );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 判断源端mongodb部署模式
        CommLib.checkMongoMode( CommLib.confFileIncr, ssh, mongoClient );

        // 将数据迁移至目标端
        CommLib.genSequoiaShakeBackground1( CommLib.confFileIncr, ssh );
        MongoCollection< Document > collection = mongoClient.getDatabase( cs )
                .getCollection( cl );
        collection.updateOne( Filters.eq( "id", 1 ),
                new Document( "$set", new Document( "int32", 1 ) ),
                new UpdateOptions().upsert( true ) );
        collection.updateOne( Filters.eq( "id", 2 ),
                new Document( "$set", new Document( "int32", 2 ) ),
                new UpdateOptions().upsert( true ) );
        collection.updateOne( Filters.eq( "id", 3 ),
                new Document( "$set", new Document( "int32", 3 ) ),
                new UpdateOptions().upsert( true ) );

        // 判断增量同步是否完成
        CommLib.checkIncr( ssh );

        // 对比源端和目标端数据
        CommLib.checkRecordEqual( cs, cl, mongoClient, sequoiaClient );
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
