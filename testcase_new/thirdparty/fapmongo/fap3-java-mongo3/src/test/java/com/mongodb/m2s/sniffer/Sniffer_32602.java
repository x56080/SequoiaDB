package com.mongodb.m2s.sniffer;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.*;
import com.mongodb.client.model.InsertManyOptions;
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
 * @Descreption seqDB-32602:执行基本增删改查操作，检测消息分析报告
 * @Author tangtao
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/17
 * @UpdateRemark
 * @Version
 */

public class Sniffer_32602 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32602";
    private String collectionName = "coll_32602";

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
        for ( int i = 0; i < 100; i++ ) {
            cl.insertOne( new Document( "name", "MongoDB" ).append( "count", i )
                    .append( "versions",
                            Arrays.asList( "v3.2", "v3.0", "v2.6" ) )
                    .append( "info",
                            new Document( "x", i * 2 ).append( "y", i * 3 ) ) );
        }
        for ( int i = 0; i < 50; i++ ) {
            ArrayList< Document > docs = new ArrayList<>();

            for ( int j = 0; j < 100; j++ ) {
                docs.add( new Document( "name", "MongoDB" ).append( "count", j )
                        .append( "versions",
                                Arrays.asList( "v3.2", "v3.0", "v2.6" ) )
                        .append( "info", new Document( "x", j * 2 ).append( "y",
                                j * 3 ) ) );
            }
            cl.insertMany( docs, new InsertManyOptions().ordered( true )
                    .bypassDocumentValidation( true ) );
        }

        // query data
        for ( int i = 0; i < 50; i++ ) {
            FindIterable< Document > findIt = cl.find();
            MongoCursor< Document > cursor = findIt.iterator();
            if ( cursor.hasNext() ) {
                Document doc = cursor.next();
            }
        }
        for ( int i = 0; i < 50; i++ ) {
            FindIterable< Document > findIt = cl
                    .find( new Document( "count", new Document( "$lt", 50 ) ) );
            MongoCursor< Document > cursor = findIt.iterator();
            if ( cursor.hasNext() ) {
                Document doc = cursor.next();
            }
        }
        for ( int i = 0; i < 50; i++ ) {
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
        for ( int i = 0; i < 50; i++ ) {
            cl.updateOne( new Document( "count", i ),
                    new Document( "$inc", new Document( "updateNum", 1 ) ) );
        }
        for ( int i = 0; i < 50; i++ ) {
            cl.updateMany( new Document( "count", new Document( "$lt", i ) ),
                    new Document( "$inc", new Document( "updateNum", 1 ) ) );
        }
        for ( int i = 0; i < 50; i++ ) {
            cl.updateMany(
                    new Document( "$and", Arrays.asList(
                            new Document( "count", new Document( "$lt", 50 ) ),
                            new Document( "info.x",
                                    new Document( "$lt", 500 ) ) ) ),
                    new Document( "$inc", new Document( "updateNum", 1 ) ) );
        }

        // delete data
        for ( int i = 0; i < 50; i++ ) {
            cl.deleteOne( new Document( "count", i ) );
        }
        for ( int i = 0; i < 50; i++ ) {
            cl.deleteMany( new Document( "count", new Document( "$lt", i ) ) );
        }
        for ( int i = 0; i < 50; i++ ) {
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

            switch ( msg.getString( "databaseCmd" ) ) {
            case "insert": {
                // insert params
                Param msgParam = new Param();

                ArrayList< Param > subParams = new ArrayList<>();
                subParams.add( new Param( "bypassDocumentValidation", 50 ) );
                subParams.add( new Param( "ordered", 150 ) );
                Param dbParam = new Param( "databaseCmdParameters", 0,
                        subParams );

                Param opParam = new Param();
                Param.checkRecordParams( msg, "insert", 150, msgParam, dbParam,
                        opParam );
                break;
            }
            case "find": {
                // find params
                Param msgParam = new Param();

                ArrayList< Param > subParams = new ArrayList<>();
                subParams.add( new Param( "find", 150 ) );
                subParams.add( new Param( "filter", 100 ) );
                Param dbParam = new Param( "databaseCmdParameters", 0,
                        subParams );

                /*
                  "operators": [
                    {
                      "param": "filter",
                      "operators": [
                        {
                          "operator": "$or",
                          "count": 50,
                          "subOperators": [
                            {
                              "operator": "$lt",
                              "count": 100,
                              "subOperators": []
                            }
                          ]
                        },
                        {
                          "operator": "$lt",
                          "count": 50,
                          "subOperators": []
                        }
                      ]
                    }
                  ]
                 */
                Param filter_or_ltParam = new Param( "$lt", 100 );
                Param filter_orParam = new Param( "$or", 50 );
                filter_orParam.addSubParam( filter_or_ltParam );
                Param filter_ltParam = new Param( "$lt", 50 );
                Param filterParam = new Param( "filter", 0 );
                filterParam.addSubParam( filter_orParam );
                filterParam.addSubParam( filter_ltParam );
                Param opParam = new Param( "operators", 0 );
                opParam.addSubParam( filterParam );

                Param.checkRecordParams( msg, "find", 150, msgParam, dbParam,
                        opParam );
                break;
            }
            case "update": {
                Param msgParam = new Param();

                ArrayList< Param > subParams = new ArrayList<>();
                subParams.add( new Param( "updates.multi", 100 ) );
                subParams.add( new Param( "ordered", 150 ) );
                subParams.add( new Param( "updates.q", 150 ) );
                Param dbParam = new Param( "databaseCmdParameters", 0,
                        subParams );

                /*
                    "operators": [
                    {
                      "param": "updates.u",
                      "operators": [
                        {
                          "operator": "$inc",
                          "count": 150,
                          "subOperators": []
                        }
                      ]
                    },
                    {
                      "param": "updates.q",
                      "operators": [
                        {
                          "operator": "$and",
                          "count": 50,
                          "subOperators": [
                            {
                              "operator": "$lt",
                              "count": 100,
                              "subOperators": []
                            }
                          ]
                        },
                        {
                          "operator": "$lt",
                          "count": 50,
                          "subOperators": []
                        }
                      ]
                    }
                  ]
                 */
                Param updatesu_incParam = new Param( "$inc", 150 );
                Param updatesuParam = new Param( "updates.u", 0 );
                updatesuParam.addSubParam( updatesu_incParam );
                Param updatesq_and_ltParam = new Param( "$lt", 100 );
                Param updatesq_andParam = new Param( "$and", 50 );
                updatesq_andParam.addSubParam( updatesq_and_ltParam );
                Param updatesq_ltParam = new Param( "$lt", 50 );
                Param updatesqParam = new Param( "updates.q", 0 );
                updatesqParam.addSubParam( updatesq_andParam );
                updatesqParam.addSubParam( updatesq_ltParam );
                Param opParam = new Param( "operators", 0 );
                opParam.addSubParam( updatesuParam );
                opParam.addSubParam( updatesqParam );
                Param.checkRecordParams( msg, "update", 150, msgParam, dbParam,
                        opParam );
                break;
            }
                case "delete": {
                    Param msgParam = new Param();

                    ArrayList< Param > subParams = new ArrayList<>();
                    subParams.add( new Param( "deletes", 150 ) );
                    subParams.add( new Param( "deletes.q", 150 ) );
                    Param dbParam = new Param( "databaseCmdParameters", 0,
                            subParams );

                /*
                  "operators": [
                    {
                      "param": "deletes.q",
                      "operators": [
                        {
                          "operator": "$and",
                          "count": 50,
                          "subOperators": [
                            {
                              "operator": "$gt",
                              "count": 100,
                              "subOperators": []
                            }
                          ]
                        },
                        {
                          "operator": "$lt",
                          "count": 50,
                          "subOperators": []
                        }
                      ]
                    }
                  ]
                 */
                    Param deletesq_and_gtParam = new Param( "$gt", 100 );
                    Param deletesq_andParam = new Param( "$and", 50 );
                    deletesq_andParam.addSubParam( deletesq_and_gtParam );
                    Param deletesq_ltParam = new Param( "$lt", 50 );
                    Param deletesqParam = new Param( "deletes.q", 0 );
                    deletesqParam.addSubParam( deletesq_andParam );
                    deletesqParam.addSubParam( deletesq_ltParam );
                    Param opParam = new Param( "operators", 0 );
                    opParam.addSubParam( deletesqParam );
                    Param.checkRecordParams( msg, "delete", 150, msgParam, dbParam,
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
