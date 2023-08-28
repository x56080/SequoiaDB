package com.mongodb.m2s.sniffer;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.*;
import com.mongodb.client.model.FindOneAndUpdateOptions;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.m2s.testcommon.Param;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;


/**
 * @Descreption seqDB-32608:执行explain 命令查看执行计划，检测消息分析报告
 *              seqDB-32609:执行aggregate 进行聚合查询
 * @Author tangtao
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */

public class Sniffer_32608_32609 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32608_32609";
    private String collectionName = "coll_32608_32609";

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


        // explain query
        database.runCommand(new Document( "explain", new Document( "find", collectionName )
                .append( "filter", new Document( "count", new Document( "$lt", 50 ) )
                        .append( "name", new Document( "$eq", "MongoDB" ) ) ) ) );

        database.runCommand(new Document( "explain", new Document( "find", collectionName )
                .append( "filter", new Document( "$or", Arrays.asList(
                        new Document( "count", new Document( "$lt", 50 ) ),
                        new Document( "info.x",
                                new Document( "$lt", 500 ) ) ) ) ) ) );


        // aggregate data
        List< Document > pipeline = Arrays.asList(
                new Document( "$match",
                        new Document( "count", new Document( "$lt", 50 ) ) ),
                new Document( "$group", new Document( "_id", "$name" ).append(
                        "count", new Document( "$sum", "$count" ) ) ) );
        AggregateIterable< Document > output = cl.aggregate( pipeline );
        Iterator< Document > iterator = output.iterator();
        if ( iterator.hasNext() ) {
            Document doc = iterator.next();
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
                case "explain.find": {
                    // insert params
                    Param msgParam = new Param();

                    ArrayList< Param > subParams = new ArrayList<>();
                    subParams.add( new Param( "find.find", 2 ) );
                    subParams.add( new Param( "find.filter", 2 ) );
                    subParams.add( new Param( "explain.explain", 2 ) );
                    Param dbParam = new Param( "databaseCmdParameters", 0,
                            subParams );

                /*
                  "operators": [
                    {
                      "param": "find.filter",
                      "operators": [
                        {
                          "operator": "$eq",
                          "count": 1,
                          "subOperators": []
                        },
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
                    Param filter_or_ltParam = new Param( "$lt", 2 );
                    Param filter_orParam = new Param( "$or", 1 );
                    filter_orParam.addSubParam( filter_or_ltParam );
                    Param filter_ltParam = new Param( "$lt", 1 );
                    Param filter_eqParam = new Param( "$eq", 1 );
                    Param filterParam = new Param( "find.filter", 0 );
                    filterParam.addSubParam( filter_orParam );
                    filterParam.addSubParam( filter_ltParam );
                    filterParam.addSubParam( filter_eqParam );
                    Param opParam = new Param( "operators", 0 );
                    opParam.addSubParam( filterParam );
                    Param.checkRecordParams( msg, "explain.find", 2, msgParam, dbParam,
                            opParam );
                    break;
                }
                case "aggregate": {
                    Param msgParam = new Param();

                    ArrayList< Param > subParams = new ArrayList<>();
                    subParams.add( new Param( "aggregate", 1 ) );
                    subParams.add( new Param( "pipeline", 1 ) );
                    Param dbParam = new Param( "databaseCmdParameters", 0,
                            subParams );

                /*
                  "operators": [
                    {
                      "param": "pipeline",
                      "operators": [
                        {
                          "operator": "$group",
                          "count": 1,
                          "subOperators": [
                            {
                              "operator": "$sum",
                              "count": 1,
                              "subOperators": []
                            }
                          ]
                        },
                        {
                          "operator": "$match",
                          "count": 1,
                          "subOperators": [
                            {
                              "operator": "$lt",
                              "count": 1,
                              "subOperators": []
                            }
                          ]
                        }
                      ]
                    }
                  ]
                 */
                    Param pipeline_group_sumParam = new Param( "$sum", 1 );
                    Param pipeline_groupParam = new Param( "$group", 1 );
                    pipeline_groupParam.addSubParam( pipeline_group_sumParam );
                    Param pipeline_match_ltParam = new Param( "$lt", 1 );
                    Param pipeline_matchParam = new Param( "$match", 1 );
                    pipeline_matchParam.addSubParam( pipeline_match_ltParam );
                    Param pipelineParam = new Param( "pipeline", 0 );
                    pipelineParam.addSubParam( pipeline_groupParam );
                    pipelineParam.addSubParam( pipeline_matchParam );
                    Param opParam = new Param( "operators", 0 );
                    opParam.addSubParam( pipelineParam );

                    Param.checkRecordParams( msg, "aggregate", 1, msgParam, dbParam,
                            opParam );
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
