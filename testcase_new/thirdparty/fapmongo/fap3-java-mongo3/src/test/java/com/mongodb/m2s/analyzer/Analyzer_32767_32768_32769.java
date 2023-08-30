package com.mongodb.m2s.analyzer;

import com.alibaba.fastjson.JSONArray;
import com.mongodb.WriteConcern;
import com.mongodb.client.*;
import org.bson.Document;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONObject;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

import java.util.*;

/**
 * @Descreption seqDB-32767:收集op_insert类型消息，分析消息报告
 *              seqDB-32768:收集op_update类型消息，分析消息报告
 *              seqDB-32769:收集op_delete类型消息，分析消息报告
 * @Author tangtao
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32767_32768_32769 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32767_32768_32769";
    private String collectionName = "col_32767_32768_32769";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        String version = CommLib.getMongoDBVersion( mongoClient );
        if ( CommLib.compareVersion( version, "3.3" ) > 0 ) {
            throw new SkipException(
                    "this test is only for mongodb version 3.2" );
        }
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

        // create collection
        MongoDatabase database = snifferClient.getDatabase( databaseName )
                .withWriteConcern( WriteConcern.UNACKNOWLEDGED );

        database.createCollection( collectionName );
        MongoCollection< Document > cl = database
                .getCollection( collectionName );

        // insert data
        for ( int i = 0; i < 10; i++ ) {
            Thread.sleep( 10 );
            cl.insertOne( new Document( "name", "MongoDB" ).append( "count", i )
                    .append( "versions",
                            Arrays.asList( "v3.2", "v3.0", "v2.6" ) )
                    .append( "info",
                            new Document( "x", i * 2 ).append( "y", i * 3 ) ) );
        }

        // query data
        for ( int i = 0; i < 10; i++ ) {
            Thread.sleep( 10 );
            FindIterable< Document > findIt = cl.find();
            MongoCursor< Document > cursor = findIt.iterator();
            if ( cursor.hasNext() ) {
                Document doc = cursor.next();
            }
        }
        for ( int i = 0; i < 10; i++ ) {
            Thread.sleep( 10 );
            FindIterable< Document > findIt = cl
                    .find( new Document( "count", new Document( "$lt", 50 ) ) );
            MongoCursor< Document > cursor = findIt.iterator();
            if ( cursor.hasNext() ) {
                Document doc = cursor.next();
            }
        }
        for ( int i = 0; i < 10; i++ ) {
            Thread.sleep( 10 );
            FindIterable< Document > findIt = cl.find( new Document( "$or",
                    Arrays.asList(
                            new Document( "count", new Document( "$lt", 50 ) ),
                            new Document( "info.x",
                                    new Document( "$lt", 500 ) ) ) ) );
            MongoCursor< Document > cursor = findIt.iterator();
            if ( cursor.hasNext() ) {
                Document doc = cursor.next();
            }
        }

        // update data
        for ( int i = 0; i < 10; i++ ) {
            Thread.sleep( 10 );
            cl.updateOne( new Document( "count", i ),
                    new Document( "$inc", new Document( "updateNum", 1 ) ) );
        }
        for ( int i = 0; i < 10; i++ ) {
            Thread.sleep( 10 );
            cl.updateMany( new Document( "count", new Document( "$lt", i ) ),
                    new Document( "$inc", new Document( "updateNum", 1 ) ) );
        }
        for ( int i = 0; i < 10; i++ ) {
            Thread.sleep( 10 );
            cl.updateMany(
                    new Document( "$and", Arrays.asList(
                            new Document( "count", new Document( "$lt", 50 ) ),
                            new Document( "info.x",
                                    new Document( "$lt", 500 ) ) ) ),
                    new Document( "$inc", new Document( "updateNum", 1 ) ) );
        }

        // delete data
        for ( int i = 0; i < 10; i++ ) {
            Thread.sleep( 10 );
            cl.deleteOne( new Document( "count", i ) );
        }
        for ( int i = 0; i < 10; i++ ) {
            Thread.sleep( 10 );
            cl.deleteMany( new Document( "count", new Document( "$lt", i ) ) );
        }
        for ( int i = 0; i < 10; i++ ) {
            Thread.sleep( 10 );
            cl.deleteMany( new Document( "$and", Arrays.asList(
                    new Document( "count", new Document( "$gt", 50 ) ),
                    new Document( "info.x", new Document( "$gt", 500 ) ) ) ) );
        }

        // stop sniffer and analyze the message file
        CommLib.snifferStopAndAnalyze( ssh );

        // 分析sniffer消息报告
        CommLib.analyzeSnifferMsgJson( ssh );

        // 分析结果校验
        ssh.exec( "cat " + analyzerOutputPath + "sniffer.json" );
        JSONArray messages = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < messages.size(); i++ ) {
            JSONObject message = messages.getJSONObject( i );
            String opCode = message.getString( "opCode" );
            switch ( opCode ) {
            case "OP_UPDATE":
            case "OP_DELETE":
                Assert.assertEquals( message.getString( "databaseCmd" ), "" );
                Assert.assertEquals( message.getIntValue( "count" ), 30 );
                Assert.assertEquals( message.getString( "SequoiaDB" ), "N" );
                Assert.assertEquals( message.getString( "Fap" ), "N" );
                break;
            case "OP_INSERT":
                Assert.assertEquals( message.getString( "databaseCmd" ), "" );
                Assert.assertEquals( message.getIntValue( "count" ), 10 );
                Assert.assertEquals( message.getString( "SequoiaDB" ), "N" );
                Assert.assertEquals( message.getString( "Fap" ), "N" );
                break;
            default:
                continue;
            }
        }
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
