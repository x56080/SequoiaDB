package com.mongodb.m2s.sniffer;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.*;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Arrays;

/**
 * @Descreption seqDB-32916:工具指定消息格式版本
 * @Author tangtao
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */

public class Sniffer_32916 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32916";
    private String collectionName = "coll_32916";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, snifferOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {

        String capturePath = toolRootPath + "m2s-sniffer/capture/*";
        ssh.exec( "rm -rf " + capturePath );

        String snifferCommand = snifferPath + " server-start -l "
                + snifferListenPort + " -m " + snifferAddr + " -f 3.2.22,4,0";
        ssh.exec( snifferCommand );

        // create collection
        MongoClient snifferClient = CommLib.getSnifferClient();
        MongoDatabase database = snifferClient.getDatabase( databaseName );

        database.createCollection( collectionName );
        MongoCollection< Document > cl = database
                .getCollection( collectionName );

        // insert data
        for ( int i = 0; i < 50; i++ ) {
            cl.insertOne( new Document( "name", "MongoDB" ).append( "count", i )
                    .append( "versions",
                            Arrays.asList( "v3.2", "v3.0", "v2.6" ) )
                    .append( "info",
                            new Document( "x", i * 2 ).append( "y", i * 3 ) ) );
        }

        // query data
        for ( int i = 0; i < 50; i++ ) {
            FindIterable< Document > findIt = cl
                    .find( new Document( "count", new Document( "$lt", 50 ) ) );
            MongoCursor< Document > cursor = findIt.iterator();
            if ( cursor.hasNext() ) {
                Document doc = cursor.next();
            }
        }

        // update data
        for ( int i = 0; i < 50; i++ ) {
            cl.updateMany( new Document( "count", new Document( "$lt", i ) ),
                    new Document( "$inc", new Document( "updateNum", 1 ) ) );
        }

        // delete data
        for ( int i = 0; i < 50; i++ ) {
            cl.deleteMany( new Document( "count", new Document( "$lt", i ) ) );
        }

        // stop sniffer and analyze the message file
        CommLib.snifferStopAndAnalyze( ssh );

        // 分析结果校验
        ssh.exec( "cat " + snifferOutputPath + "*.json" );
        JSONArray messages = getJsonArray( ssh.getStdout() );
        for ( int i = 0; i < messages.size(); i++ ) {
            JSONObject msg = messages.getJSONObject( i );
            System.out.println( msg.toString() );

            switch ( msg.getString( "databaseCmd" ) ) {
            case "insert":
            case "find":
            case "update":
            case "delete": {
                Assert.assertEquals( msg.getString( "opCode" ), "OP_QUERY" );
                Assert.assertEquals( msg.get( "count" ), 50,
                        msg.getString( "opCode" )
                                + " param count is not equal" );
                break;
            }
            default:
                continue;
            }

        }

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, snifferOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
        if ( mongoClient != null )
            mongoClient.close();
        if ( ssh != null )
            ssh.disconnect();
    }

    // 输入的str有多行，每行一个json，返回一个json数组
    private JSONArray getJsonArray( String str ) {
        JSONArray jsonArray = new JSONArray();
        String[] lines = str.split( "\n" );
        for ( String line : lines ) {
            jsonArray.add( JSONObject.parseObject( line ) );
        }
        return jsonArray;
    }
}
