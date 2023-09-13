package com.mongodb.m2s.sniffer;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.*;
import com.mongodb.client.model.UpdateOptions;
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
 * @Descreption seqDB-32603:执行查询操作，查询语句指定参数，检测消息分析报告
 *              seqDB-32604:执行更新操作，更新语句指定参数，检测消息分析报告
 * @Author tangtao
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/18
 * @UpdateRemark
 * @Version
 */

public class Sniffer_32603_32604 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32603_32604";
    private String collectionName = "coll_32603_32604";

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

        // query data
        FindIterable< Document > findIt = cl
                .find( new Document( "count", new Document( "$lt", 50 ) )
                        .append( "name", new Document( "$eq", "MongoDB" ) ) );
        MongoCursor< Document > cursor = findIt.iterator();
        if ( cursor.hasNext() ) {
            Document doc = cursor.next();
        }

        findIt = cl.find( new Document( "$or", Arrays.asList(
                new Document( "count", new Document( "$lt", 50 ) ),
                new Document( "info.x", new Document( "$lt", 500 ) ) ) ) );
        cursor = findIt.iterator();
        if ( cursor.hasNext() ) {
            Document doc = cursor.next();
        }

        findIt = cl
                .find( new Document( "$or", Arrays.asList(
                        new Document( "count", new Document( "$lt", 50 ) ),
                        new Document( "info.x",
                                new Document( "$lt", 500 ) ) ) ) )
                .sort( new Document( "count", 1 ) ).limit( 10 );
        cursor = findIt.iterator();
        if ( cursor.hasNext() ) {
            Document doc = cursor.next();
        }

        // update data
        cl.updateMany( new Document( "count", new Document( "$lt", 100 ) ),
                new Document( "$inc", new Document( "updateNum", 1 ) ) );

        cl.updateMany(
                new Document( "$and", Arrays.asList(
                        new Document( "count", new Document( "$lt", 50 ) ),
                        new Document( "info.x",
                                new Document( "$lt", 500 ) ) ) ),
                new Document( "$inc", new Document( "updateNum", 1 ) ) );

        cl.updateMany( new Document( "count", new Document( "$lt", 100 ) ),
                new Document( "$set", new Document( "updateNum", 1 ) ),
                new UpdateOptions().bypassDocumentValidation( true )
                        .upsert( true ) );


        // stop sniffer and analyze the message file
        CommLib.snifferStopAndAnalyze( ssh );

        // 分析结果校验
        ssh.exec( "cat " + snifferOutputPath + "*.json" );
        JSONArray messages = getJsonArray( ssh.getStdout() );
        for ( int i = 0; i < messages.size(); i++ ) {
            JSONObject msg = messages.getJSONObject( i );
            System.out.println( msg.toString() );

            switch ( msg.getString( "databaseCmd" ) ) {
                case "find":
                    checkFindInfo( msg );
                    break;
                case "update":
                    checkUpdateInfo( msg );
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

    private void checkFindInfo( JSONObject msg ){
        // find params
        Param msgParam = new Param();

        ArrayList< Param > subParams = new ArrayList<>();
        subParams.add( new Param( "find", 3 ) );
        subParams.add( new Param( "filter", 3 ) );
        subParams.add( new Param( "limit", 1 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParams );

        /*
          "operators": [
            {
              "param": "filter",
              "operators": [
                {
                  "operator": "$eq",
                  "count": 1,
                  "subOperators": []
                },
                {
                  "operator": "$or",
                  "count": 2,
                  "subOperators": [
                    {
                      "operator": "$lt",
                      "count": 4,
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
        Param filter_eqParam = new Param( "$eq", 1 );
        Param filter_or_ltParam = new Param( "$lt", 4 );
        Param filter_orParam = new Param( "$or", 2 );
        filter_orParam.addSubParam( filter_or_ltParam );
        Param filter_ltParam = new Param( "$lt", 1 );
        Param filterParam = new Param( "filter", 0 );
        filterParam.addSubParam( filter_eqParam );
        filterParam.addSubParam( filter_orParam );
        filterParam.addSubParam( filter_ltParam );
        Param opParam = new Param( "operators", 0 );
        opParam.addSubParam( filterParam );

        Param.checkRecordParams( msg, "find", 3, msgParam, dbParam,
                opParam );
    }

    private void checkUpdateInfo( JSONObject msg ){
        Param msgParam = new Param();

        ArrayList< Param > subParams = new ArrayList<>();
        subParams.add( new Param( "updates.multi", 3 ) );
        subParams.add( new Param( "bypassDocumentValidation", 1 ) );
        subParams.add( new Param( "updates.upsert", 1 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParams );

        /*
          "operators": [
            {
              "param": "updates.u",
              "operators": [
                {
                  "operator": "$inc",
                  "count": 2,
                  "subOperators": []
                },
                {
                  "operator": "$set",
                  "count": 1,
                  "subOperators": []
                }
              ]
            },
            {
              "param": "updates.q",
              "operators": [
                {
                  "operator": "$and",
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
                  "count": 2,
                  "subOperators": []
                }
              ]
            }
          ]
         */
        Param updatesu_incParam = new Param( "$inc", 2 );
        Param updatesu_setParam = new Param( "$set", 1 );
        Param updatesuParam = new Param( "updates.u", 0 );
        updatesuParam.addSubParam( updatesu_incParam );
        updatesuParam.addSubParam( updatesu_setParam );
        Param updatesq_and_ltParam = new Param( "$lt", 2 );
        Param updatesq_andParam = new Param( "$and", 1 );
        updatesq_andParam.addSubParam( updatesq_and_ltParam );
        Param updatesq_ltParam = new Param( "$lt", 2 );
        Param updatesqParam = new Param( "updates.q", 0 );
        updatesqParam.addSubParam( updatesq_andParam );
        updatesqParam.addSubParam( updatesq_ltParam );
        Param opParam = new Param( "operators", 0 );
        opParam.addSubParam( updatesuParam );
        opParam.addSubParam( updatesqParam );
        Param.checkRecordParams( msg, "update", 3, msgParam, dbParam,
                opParam );
    }
}
