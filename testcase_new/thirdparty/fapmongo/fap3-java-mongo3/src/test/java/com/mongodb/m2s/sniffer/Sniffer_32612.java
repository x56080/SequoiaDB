package com.mongodb.m2s.sniffer;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.WriteConcern;
import com.mongodb.client.*;
import com.mongodb.client.model.InsertManyOptions;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * @Descreption seqDB-32612:执行基本增删改查操作，不走数据库命令，检测消息分析报告
 * @Author tangtao
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */

public class Sniffer_32612 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32612";
    private String collectionName = "coll_32612";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, snifferOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        String version = CommLib.getMongoDBVersion( mongoClient );
        if ( CommLib.compareVersion( version, "3.3" ) > 0 ) {
            throw new SkipException(
                    "this test is only for mongodb version 3.2" );
        }

        CommLib.snifferStart( ssh );

        // create collection
        MongoClient snifferClient = CommLib.getSnifferClient();
        MongoDatabase database = snifferClient.getDatabase( databaseName )
                .withWriteConcern( WriteConcern.UNACKNOWLEDGED );

        database.createCollection( collectionName );
        MongoCollection< Document > cl = database
                .getCollection( collectionName );

        // insert data
        for ( int i = 0; i < 10; i++ ) {
            cl.insertOne( new Document( "name", "MongoDB" ).append( "count", i )
                    .append( "versions",
                            Arrays.asList( "v3.2", "v3.0", "v2.6" ) )
                    .append( "info",
                            new Document( "x", i * 2 ).append( "y", i * 3 ) ) );
        }

        for ( int i = 0; i < 10; i++ ) {
            ArrayList< Document > docs = new ArrayList<>();

            for ( int j = 0; j < 100; j++ ) {
                docs.add( new Document( "name", "MongoDB" ).append( "count", j )
                        .append( "versions",
                                Arrays.asList( "v3.2", "v3.0", "v2.6" ) )
                        .append( "info", new Document( "x", j * 2 ).append( "y",
                                j * 3 ) ) );
            }
            cl.insertMany( docs );
        }

        // query data
        for ( int i = 0; i < 10; i++ ) {
            FindIterable< Document > findIt = cl.find();
            MongoCursor< Document > cursor = findIt.iterator();
            if ( cursor.hasNext() ) {
                Document doc = cursor.next();
            }
        }
        for ( int i = 0; i < 10; i++ ) {
            FindIterable< Document > findIt = cl
                    .find( new Document( "count", new Document( "$lt", 50 ) ) );
            MongoCursor< Document > cursor = findIt.iterator();
            if ( cursor.hasNext() ) {
                Document doc = cursor.next();
            }
        }
        for ( int i = 0; i < 10; i++ ) {
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
            cl.updateOne( new Document( "count", i ),
                    new Document( "$inc", new Document( "updateNum", 1 ) ) );
        }
        for ( int i = 0; i < 10; i++ ) {
            cl.updateMany( new Document( "count", new Document( "$lt", i ) ),
                    new Document( "$inc", new Document( "updateNum", 1 ) ) );
        }
        for ( int i = 0; i < 10; i++ ) {
            cl.updateMany(
                    new Document( "$and", Arrays.asList(
                            new Document( "count", new Document( "$lt", 50 ) ),
                            new Document( "info.x",
                                    new Document( "$lt", 500 ) ) ) ),
                    new Document( "$inc", new Document( "updateNum", 1 ) ) );
        }

        // delete data
        for ( int i = 0; i < 10; i++ ) {
            cl.deleteOne( new Document( "count", i ) );
        }
        for ( int i = 0; i < 10; i++ ) {
            cl.deleteMany( new Document( "count", new Document( "$lt", i ) ) );
        }
        for ( int i = 0; i < 10; i++ ) {
            cl.deleteMany( new Document( "$and", Arrays.asList(
                    new Document( "count", new Document( "$gt", 50 ) ),
                    new Document( "info.x", new Document( "$gt", 500 ) ) ) ) );
        }

        // stop sniffer and analyze the message file
        CommLib.snifferStopAndAnalyze( ssh );

        // 分析结果校验
        ssh.exec( "cat " + snifferOutputPath + "*.json" );
        JSONArray messages = getJsonArray( ssh.getStdout() );
        for ( int i = 0; i < messages.size(); i++ ) {
            JSONObject msg = messages.getJSONObject( i );
            System.out.println( msg.toString() );

            switch ( msg.getString( "opCode" ) ) {
            case "OP_INSERT": {
                Assert.assertEquals( msg.get( "count" ), 1010,
                        msg.getString( "opCode" )
                                + " param count is not equal" );
                break;
            }
            case "OP_QUERY": {
                if ( msg.getString( "databaseCmd" ).equals( "find" ) ) {
                    Assert.assertEquals( msg.get( "count" ), 30,
                            msg.getString( "opCode" )
                                    + " param count is not equal" );
                }
                break;
            }
            case "OP_UPDATE":
            case "OP_DELETE": {
                Assert.assertEquals( msg.get( "count" ), 30,
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
    JSONArray getJsonArray( String str ) {
        JSONArray jsonArray = new JSONArray();
        String[] lines = str.split( "\n" );
        for ( String line : lines ) {
            jsonArray.add( JSONObject.parseObject( line ) );
        }
        return jsonArray;
    }
}
