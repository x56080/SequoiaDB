package com.mongodb.m2s.analyzer;

import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import org.bson.Document;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoDatabase;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * @Descreption seqDB-32764:分析环境信息文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32764 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32764";
    private String collectionName = "col_32764";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, snifferOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        // 启动sniffer工具
        CommLib.snifferStart( ssh );
        // 连接sniffer监听端口
        MongoClient snifferClient = CommLib.getSnifferClient();
        // 执行简单操作
        MongoDatabase database = snifferClient.getDatabase( databaseName );
        MongoCollection< Document > collection = database
                .getCollection( collectionName );
        // 插入数据
        for ( int i = 0; i < 100; i++ ) {
            collection.insertOne( new Document( "name", "test" + i ) );
        }

        // 获取捕获到的环境信息
        String version = null;
        Map< String, ArrayList > map = new HashMap<>();
        ArrayList< String > clients = new ArrayList<>();
        ArrayList< String > servers = new ArrayList<>();
        ssh.exec( "cat " + toolRootPath + "m2s-sniffer/capture/env.json" );
        String[] split = ssh.getStdout().split( "\n" );
        for ( String str : split ) {
            JSONObject jsonObject = JSONObject.parseObject( str );
            if ( jsonObject.containsKey( "sniffer" ) ) {
                version = jsonObject.getJSONObject( "sniffer" )
                        .getString( "version" );
            }
            if ( jsonObject.containsKey( "client" ) ) {
                JSONObject driver = jsonObject.getJSONObject( "client" )
                        .getJSONObject( "driver" );
                clients.add( driver.getString( "name" ) + " "
                        + driver.getString( "version" ) );
            }
            if ( jsonObject.containsKey( "server" ) ) {
                JSONObject server = jsonObject.getJSONObject( "server" );
                servers.add( server.getString( "version" ) );
            }
        }
        map.put( "client", clients );
        map.put( "server", servers );

        // 分析sniffer环境报告
        CommLib.analyzeSnifferEnvJson( ssh );

        // 分析结果校验index.json
        ssh.exec( "cat " + analyzerOutputPath + "summary.json" );
        JSONObject summary = JSONObject.parseObject( ssh.getStdout() );
        summary.getJSONArray( "MongoDBDriver" )
                .containsAll( map.get( "client" ) );
        summary.getJSONArray( "MongoDBServer" )
                .containsAll( map.get( "server" ) );
        summary.getString( "m2s-sniffer" ).equals( version );

        CommLib.snifferStopAndAnalyze( ssh );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, snifferOutputPath );
        CommLib.rmDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
        if ( mongoClient != null )
            mongoClient.close();
        if ( ssh != null )
            ssh.disconnect();
    }

}
