package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * @Descreption seqDB-32693 :: 版本: 1 :: 增量同步过程更新数据
 * @Author wangxingming
 * @CreateDate 2023/8/8
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/8
 * @Version 1.0
 */
public class SequoiaShake32693 {
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
        final Ssh ssh = CommLib.createSsh();

        // 获取当前checkpoint并修改conf文件
        CommLib.updateCheckpoint( CommLib.confFileIncr, ssh );

        // 构造数据
        CommLib.genDataToMongoDB( configFile, workerNum, ssh );

        // 判断源端mongodb部署模式
        CommLib.checkMongoMode( CommLib.confFileIncr, ssh, mongoClient );

        // 将数据迁移至目标端
        Callable< Void > task1 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                CommLib.genSequoiaShakeBackground( CommLib.confFileIncr, ssh );
                return null;
            }
        };
        Callable< Void > task2 = new Callable< Void >() {
            @Override
            public Void call() throws Exception {
                MongoClient mongoClient = MongoClients.create( "mongodb://"
                        + CommLib.MongoDBHost + ":" + CommLib.MongoDBPort );
                MongoCollection< Document > collection = mongoClient
                        .getDatabase( cs ).getCollection( cl );
                // 插入字符串类型数据
                Document stringDoc = new Document( "stringField",
                        "Hello World" );
                collection.insertOne( stringDoc );
                // 插入int32类型数据，覆盖边界值
                Document int32Doc = new Document( "maxInt32Field",
                        Integer.MAX_VALUE ).append( "minInt32Field",
                                Integer.MIN_VALUE );
                collection.insertOne( int32Doc );
                // 插入int64类型数据，覆盖边界值
                Document int64Doc = new Document( "maxInt64Field",
                        Long.MAX_VALUE ).append( "minInt64Field",
                                Long.MIN_VALUE );
                collection.insertOne( int64Doc );
                // 插入float类型数据，覆盖边界值
                Document floatDoc = new Document( "maxFloatField",
                        Float.MAX_VALUE ).append( "minFloatField",
                                Float.MIN_VALUE );
                collection.insertOne( floatDoc );
                // 插入boolean类型数据
                Document booleanDoc = new Document( "booleanField1", true )
                        .append( "booleanField2", false );
                collection.insertOne( booleanDoc );
                // 插入Decimal类型数据，覆盖精度
                Document decimalDoc = new Document( "decimalField",
                        new java.math.BigDecimal( "123.4567890123456789" ) );
                collection.insertOne( decimalDoc );
                // 插入包含特殊字符的字符串类型数据
                Document specialCharsDoc = new Document( "specialCharsField",
                        "!@#$%^&*()" );
                collection.insertOne( specialCharsDoc );
                // 插入中文字符串类型数据
                Document chineseDoc = new Document( "chineseField", "你好，世界" );
                collection.insertOne( chineseDoc );
                // 插入空串字符串类型数据
                Document emptyStringDoc = new Document( "emptyStringField",
                        "" );
                collection.insertOne( emptyStringDoc );
                mongoClient.close();
                return null;
            }
        };
        ExecutorService executorService = Executors.newFixedThreadPool( 2 );
        List< Callable< Void > > tasks = Arrays.asList( task1, task2 );
        executorService.invokeAll( tasks );
        executorService.shutdown();

        // 对比源端和目标端数据
        CommLib.checkDataResult( "all", "config,sequoiashake", ssh );

        // 关闭SSH连接
        ssh.disconnect();
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            CommLib.dropDatabase( cs, mongoClient, sequoiaClient );
        } finally {
            mongoClient.close();
            sequoiaClient.close();
        }
    }
}
