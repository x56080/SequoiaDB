package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.utils.Ssh;
import org.bson.BsonTimestamp;
import org.bson.Document;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

/**
 * @Descreption 测试用例 seqDB-32630 :: 版本: 1 :: 全量同步数据为日期和时间类型
 * @Author wangxingming
 * @CreateDate 2023/8/3
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/3
 * @Version 1.0
 */
public class SequoiaShake32630 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "dateType.json";
    int workerNum = 4;
    String cs = "mgocs_date";
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
        MongoCollection< Document > collection = mongoClient.getDatabase( cs )
                .getCollection( cl );
        // 插入日期类型数据，覆盖边界值
        Document dateDoc = new Document( "maxDateField",
                new Date( Long.MAX_VALUE ) ).append( "minDateField",
                        new Date( Long.MIN_VALUE ) );
        collection.insertOne( dateDoc );
        // 插入时间戳类型数据，覆盖边界值
        Document timestampDoc = new Document( "maxTimestampField",
                new BsonTimestamp( Integer.MAX_VALUE, 0 ) ).append(
                        "minTimestampField",
                        new BsonTimestamp( Integer.MIN_VALUE, 0 ) );
        collection.insertOne( timestampDoc );

        // 将数据迁移至目标端
        CommLib.genSequoiaShake( CommLib.confFileFull, ssh );

        // 对比源端和目标端数据
        CommLib.checkDataResult( "all", "config,sequoiashake", ssh );

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