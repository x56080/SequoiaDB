package com.mongodb.m2s.analyzer;

import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoDatabase;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32764:分析环境信息文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32764 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String db_32764 = "db_32764";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, snifferOutputPath);
        CommLib.initDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( db_32764 ).drop();
    }

    @Test
    public void test() throws Exception {
        // 连接mongodb执行操作，使sniffer收集到环境信息
        MongoDatabase database = mongoClient.getDatabase( db_32764 );
        String collection = "col_32764";
        // 插入数据
        for ( int i = 0; i < 100; i++ ) {
            database.getCollection( collection )
                    .insertOne( new Document( "name", "test" + i ) );
        }
        

        // 分析集合信息
        String collectionJsonPath = snifferOutputPath + "env.json";
        CommLib.analyzeCollection( ssh, collectionJsonPath );

        // 分析结果校验index.json
        ssh.exec( "cat " + analyzerOutputPath + "summary.json" );
        boolean containIndex1 = false;
        boolean containIndex2 = false;
        // 可以正确分析到同名索引
        JSONArray indexes = JSONObject.parseArray( ssh.getStdout() );
        

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, snifferOutputPath);
        CommLib.rmDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( db_32764 ).drop();
        if ( mongoClient != null )
            mongoClient.close();
        if ( ssh != null )
            ssh.disconnect();
    }

}
