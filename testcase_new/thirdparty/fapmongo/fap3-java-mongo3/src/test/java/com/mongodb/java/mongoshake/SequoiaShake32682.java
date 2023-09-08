package com.mongodb.java.mongoshake;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.gridfs.GridFSBucket;
import com.mongodb.client.gridfs.GridFSBuckets;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.bson.types.Binary;
import org.bson.types.ObjectId;
import org.bson.types.Symbol;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.FileInputStream;

/**
 * @Descreption seqDB-32682 :: 版本: 1 :: 增量同步数据为其它数据类型
 * @Author wangxingming
 * @CreateDate 2023/8/8
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/8
 * @Version 1.0
 */
public class SequoiaShake32682 {
    MongoClient mongoClient;
    MongoClient sequoiaClient;
    String configFile = "otherType.json";
    int workerNum = 4;
    String cs = "mgocs_other";
    String cl = "mgocl";
    private CommLib commlib = new CommLib();

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
        Ssh ssh = commlib.createSsh();

        // 获取当前checkpoint并修改conf文件
        commlib.updateCheckpoint( CommLib.confFileIncr, ssh );

        // 构造数据
        commlib.genDataToMongoDB( configFile, workerNum, ssh );
        MongoCollection< Document > collection = mongoClient.getDatabase( cs )
                .getCollection( cl );
        // 插入二进制类型数据
        byte[] binaryData = "Hello, World!".getBytes();
        Document binaryDoc = new Document( "binaryField",
                new Binary( binaryData ) );
        collection.insertOne( binaryDoc );
        // 插入ObjectId类型数据
        ObjectId objectId = new ObjectId();
        Document objectIdDoc = new Document( "_id", objectId );
        collection.insertOne( objectIdDoc );
        // 插入null类型数据
        Document nullDoc = new Document( "nullField", null );
        collection.insertOne( nullDoc );
        // 插入GridFS数据
        GridFSBucket gridFSBucket = GridFSBuckets
                .create( mongoClient.getDatabase( cs ) );
        FileInputStream inputStream = new FileInputStream(
                "C:\\Users\\wangxingming\\Downloads\\mongodb.txt" );
        ObjectId fileId = gridFSBucket.uploadFromStream( "filename",
                inputStream );
        Document gridFSDoc = new Document( "gridFSField", fileId );
        collection.insertOne( gridFSDoc );
        // 插入Symbol数据
        Document symbolDoc = new Document( "symbolField",
                new Symbol( "SymbolValue" ) );
        collection.insertOne( symbolDoc );

        // 判断源端mongodb部署模式
        commlib.checkMongoMode( CommLib.confFileIncr, ssh, mongoClient );

        // 将数据迁移至目标端
        commlib.genSequoiaShakeBackground( CommLib.confFileIncr, ssh );

        // 对比源端和目标端数据
        commlib.checkDataResult( "all", "config,sequoiashake", ssh );

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
