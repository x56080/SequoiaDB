package com.mongodb.m2s.sniffer;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.*;
import com.mongodb.client.model.CountOptions;
import com.mongodb.client.model.EstimatedDocumentCountOptions;
import com.mongodb.client.model.FindOneAndUpdateOptions;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.m2s.testcommon.Param;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * @Descreption seqDB-32605:执行findAndModify 命令，检测消息分析报告
 *              seqDB-32606:执行count命令，检测消息分析报告
 *              seqDB-32607:执行distinct 命令，检测消息分析报告
 * @Author tangtao
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/17
 * @UpdateRemark
 * @Version
 */

public class Sniffer_32605_32606_32607 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32605_32606_32607";
    private String collectionName = "coll_32605_32606_32607";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, snifferOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {

        CommLib.snifferStart( ssh );

        // create collection
        MongoClient snifferClient = CommLib.getSnifferClient();
        MongoDatabase database = snifferClient.getDatabase( databaseName );
        database.createCollection( collectionName );
        MongoCollection< Document > cl = database
                .getCollection( collectionName );

        // insert data
        ArrayList< Document > docs = new ArrayList<>();
        for ( int i = 0; i < 200; i++ ) {
            docs.add( new Document( "name", "MongoDB" ).append( "count", i )
                    .append( "versions",
                            Arrays.asList( "v3.2", "v3.0", "v2.6" ) )
                    .append( "info",
                            new Document( "x", i * 2 ).append( "y", i * 3 ) ) );
        }
        cl.insertMany( docs );

        // count data
        int totalCount = 0;
        for ( int i = 0; i < 5; i++ ) {
            database.runCommand( new Document( "count", collectionName ) );
            database.runCommand( new Document( "count", collectionName )
                    .append( "query", new Document( "count",
                            new Document( "$lt", 50 ) ) ) );
            database.runCommand( new Document( "count", collectionName )
                    .append( "query", new Document( "count", -100 ) )
                    .append( "limit", 100 ) );
        }

        // find and modify data
        cl.findOneAndUpdate( new Document( "count", new Document( "$lt", 50 ) ),
                new Document( "$inc", new Document( "count", 1 ) ) );

        cl.findOneAndUpdate( new Document( "count", -100 ),
                new Document( "$inc", new Document( "count", 1 ) ),
                new FindOneAndUpdateOptions().upsert( true ) );

        cl.findOneAndDelete( new Document( "count", 500 ) );

        // distinct query
        String name = cl.distinct( "name", new Document(), String.class )
                .first();

        name += cl.distinct( "name",
                new Document( "count", new Document( "$lt", 100 ) ),
                String.class ).first();

        name += cl
                .distinct( "name", new Document( "$or", Arrays.asList(
                        new Document( "count", new Document( "$lt", 50 ) ),
                        new Document( "info.x",
                                new Document( "$lt", 500 ) ) ) ),
                        String.class )
                .first();

        // stop sniffer and analyze the message file
        CommLib.snifferStopAndAnalyze( ssh );

        // 分析结果校验
        ssh.exec( "cat " + snifferOutputPath + "*.json" );
        JSONArray messages = getJsonArray( ssh.getStdout() );
        for ( int i = 0; i < messages.size(); i++ ) {
            JSONObject msg = messages.getJSONObject( i );
            System.out.println( msg.toString() );

            switch ( msg.getString( "databaseCmd" ) ) {
            case "count":
                checkCountInfo( msg );
                break;
            case "findAndModify":
                checkFindAndModifyInfo( msg );
                break;
            case "distinct":
                checkDistinctInfo( msg );
                break;
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

    void checkCountInfo( JSONObject msg ){
        Param msgParam = new Param();

        ArrayList< Param > subParams = new ArrayList<>();
        subParams.add( new Param( "count", 15 ) );
        subParams.add( new Param( "limit", 5 ) );
        subParams.add( new Param( "query", 10 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParams );

        /*
          "operators": [
            {
              "param": "query",
              "operators": [
                {
                  "operator": "$lt",
                  "count": 5,
                  "subOperators": []
                }
              ]
            }
          ]
         */
        Param query_ltParam = new Param( "$lt", 5 );
        Param queryParam = new Param( "query", 0 );
        queryParam.addSubParam( query_ltParam );
        Param opParam = new Param( "operators", 0 );
        opParam.addSubParam( queryParam );
        Param.checkRecordParams( msg, "count", 15, msgParam, dbParam,
                opParam );
    }

    void checkFindAndModifyInfo( JSONObject msg ){
        // find params
        Param msgParam = new Param();

        ArrayList< Param > subParams = new ArrayList<>();
        subParams.add( new Param( "update", 2 ) );
        subParams.add( new Param( "remove", 1 ) );
        subParams.add( new Param( "new", 2 ) );
        subParams.add( new Param( "upsert", 1 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParams );

        /*
          "operators": [
            {
              "param": "query",
              "operators": [
                {
                  "operator": "$lt",
                  "count": 1,
                  "subOperators": []
                }
              ]
            },
            {
              "param": "update",
              "operators": [
                {
                  "operator": "$inc",
                  "count": 2,
                  "subOperators": []
                }
              ]
            }
          ]
         */
        Param query_ltParam = new Param( "$lt", 1 );
        Param queryParam = new Param( "query", 0 );
        queryParam.addSubParam( query_ltParam );
        Param update_incParam = new Param( "$inc", 2 );
        Param updateParam = new Param( "query", 0 );
        updateParam.addSubParam( update_incParam );
        Param opParam = new Param( "operators", 0 );
        opParam.addSubParam( queryParam );
        opParam.addSubParam( updateParam );

        Param.checkRecordParams( msg, "findAndModify", 3, msgParam,
                dbParam, opParam );
    }

    void checkDistinctInfo( JSONObject msg ){
        Param msgParam = new Param();

        ArrayList< Param > subParams = new ArrayList<>();
        subParams.add( new Param( "query", 2 ) );
        subParams.add( new Param( "distinct", 3 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParams );

        /*
          "operators": [
            {
              "param": "query",
              "operators": [
                {
                  "operator": "$or",
                  "count": 1,
                  "subOperators": [
                    {
                      "operator": "$lt",
                      "count": 2,
                      "subOperators": []
                    }
                  ]
                },
                {
                  "operator": "$lt",
                  "count": 1,
                  "subOperators": []
                }
              ]
            }
          ]
         */
        Param query_or_ltParam = new Param( "$lt", 2 );
        Param query_orParam = new Param( "$or", 1 );
        query_orParam.addSubParam( query_or_ltParam );
        Param query_ltParam = new Param( "$lt", 1 );
        Param queryParam = new Param( "query", 0 );
        queryParam.addSubParam( query_orParam );
        queryParam.addSubParam( query_ltParam );
        Param opParam = new Param( "operators", 0 );
        opParam.addSubParam( queryParam );
        Param.checkRecordParams( msg, "distinct", 3, msgParam, dbParam,
                opParam );
    }
}
