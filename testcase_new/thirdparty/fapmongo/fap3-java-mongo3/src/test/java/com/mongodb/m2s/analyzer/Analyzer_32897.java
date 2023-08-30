package com.mongodb.m2s.analyzer;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoDatabase;
import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

import java.util.Arrays;

/**
 * @Descreption seqDB-32897:收集explain命令消息，分析消息报告
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/28
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32897 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32897";
    private String collectionName = "col_32897";

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
        // 初始化数据
        mongoClient.getDatabase( databaseName )
                .createCollection( collectionName );
        for ( int i = 0; i < 100; i++ ) {
            mongoClient.getDatabase( databaseName )
                    .getCollection( collectionName )
                    .insertOne( new Document( "a", "name" + i ).append( "b", i )
                            .append( "c", Arrays.asList( i * 2, i * 3 ) ) );
        }
        mongoClient.getDatabase( databaseName ).getCollection( collectionName )
                .createIndex( new Document( "a", 1 ) );
        // 启动sniffer工具
        CommLib.snifferStart( ssh );
        // 连接sniffer监听端口
        MongoClient snifferClient = CommLib.getSnifferClient();

        MongoDatabase database = snifferClient.getDatabase( databaseName );
        // explain + find
        Document findDocument = new Document( "find", collectionName )
                .append( "filter", new Document( "a", "name1" ) )
                .append( "projection", new Document( "a", 1 ).append( "b", 1 ) )
                .append( "sort", new Document( "b", 1 ) ).append( "skip", 0 )
                .append( "limit", 10 ).append( "singleBatch", true );
        Document explainDocument = new Document( "explain", findDocument )
                .append( "verbosity", "queryPlanner" );
        database.runCommand( explainDocument );

        // explain + findAndModify
        Document findAndModifyDocument = new Document( "findAndModify",
                collectionName ).append( "query", new Document( "a", "name1" ) )
                        .append( "sort", new Document( "b", 1 ) )
                        .append( "update",
                                new Document( "$set",
                                        new Document( "b", 100 ) ) )
                        .append( "new", true );
        explainDocument = new Document( "explain", findAndModifyDocument )
                .append( "verbosity", "allPlansExecution" );
        database.runCommand( explainDocument );

        if ( CommLib.compareVersion( CommLib.getMongoDBVersion( mongoClient ),
                "4.0" ) >= 0 ) {
            // explain + aggregate
            Document aggregateDocument = new Document( "aggregate",
                    collectionName ).append(
                            "pipeline",
                            Arrays.asList(
                                    new Document( "$match",
                                            new Document( "a", "name1" ) ),
                                    new Document( "$group",
                                            new Document( "_id", "$b" ).append(
                                                    "count",
                                                    new Document( "$sum",
                                                            1 ) ) ) ) )
                            .append( "cursor",
                                    new Document( "batchSize", 10 ) );
            explainDocument = new Document( "explain", aggregateDocument )
                    .append( "verbosity", "executionStats" );
            database.runCommand( explainDocument );
        }

        // explain + count
        Document countDocument = new Document( "count", collectionName )
                .append( "query",
                        new Document( "a", new Document( "$gt", 0 ) ) )
                .append( "limit", 10 ).append( "skip", 1 )
                .append( "hint", new Document( "a", 1 ) )
                .append( "maxTimeMS", 1000 )
                .append( "readConcern", new Document( "level", "local" ) );
        explainDocument = new Document( "explain", countDocument )
                .append( "verbosity", "executionStats" );
        database.runCommand( explainDocument );

        // explain + distinct
        Document distinctDocument = new Document( "distinct", collectionName )
                .append( "key", "a" )
                .append( "query",
                        new Document( "a", new Document( "$gt", 0 ) ) )
                .append( "readConcern", new Document( "level", "local" ) );
        explainDocument = new Document( "explain", distinctDocument )
                .append( "verbosity", "executionStats" );
        database.runCommand( explainDocument );

        // explain + update
        Document updateDocument = new Document( "update", collectionName )
                .append( "updates", Arrays.asList(
                        new Document( "q", new Document( "a", "name1" ) )
                                .append( "u",
                                        new Document( "$set",
                                                new Document( "b", 100 ) ) )
                                .append( "multi", true ) ) )
                .append( "ordered", true )
                .append( "bypassDocumentValidation", true )
                .append( "maxTimeMS", 1000 );
        explainDocument = new Document( "explain", updateDocument )
                .append( "verbosity", "executionStats" );
        database.runCommand( explainDocument );

        // explain + delete
        Document deleteDocument = new Document( "delete", collectionName )
                .append( "deletes", Arrays.asList(
                        new Document( "q", new Document( "a", "name1" ) )
                                .append( "limit", 1 ) ) )
                .append( "ordered", true ).append( "maxTimeMS", 1000 );
        explainDocument = new Document( "explain", deleteDocument )
                .append( "verbosity", "executionStats" );
        database.runCommand( explainDocument );

        CommLib.snifferStopAndAnalyze( ssh );

        CommLib.analyzeSnifferMsgJson( ssh );
        ssh.exec( "cat " + analyzerOutputPath + "sniffer.json" );

        JSONArray cmds = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < cmds.size(); i++ ) {
            JSONObject cmd = cmds.getJSONObject( i );
            JSONArray parameters = cmd.getJSONArray( "parameters" );
            String databaseCmd = cmd.getString( "databaseCmd" );
            switch ( databaseCmd ) {
            case "explain.find":
                if ( CommLib.compareVersion(
                        CommLib.getMongoDBVersion( mongoClient ),
                        "3.6" ) >= 0 ) {
                    Assert.assertEquals( cmd.getString( "opCode" ), "OP_MSG" );
                } else {
                    Assert.assertEquals( cmd.getString( "opCode" ),
                            "OP_QUERY" );
                }
                Assert.assertEquals( cmd.getIntValue( "count" ), 1 );
                Assert.assertEquals( cmd.getString( "SequoiaDB" ), "Y" );
                Assert.assertEquals( cmd.getString( "Fap" ), "N" );
                for ( int j = 0; j < parameters.size(); j++ ) {
                    JSONObject parameter = parameters.getJSONObject( j );
                    Assert.assertEquals( parameter.getIntValue( "count" ), 1 );
                    String parameterName = parameter.getString( "param" );
                    switch ( parameterName ) {
                    case "find.find":
                    case "find.filter":
                    case "find.sort":
                    case "find.projection":
                    case "find.skip":
                    case "find.limit":
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals(
                                    parameter.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals(
                                    parameter.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        break;
                    case "find.singleBatch":
                    case "explain.verbosity":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "N" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    case "explain.explain":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    default:
                        continue;
                    }
                }
                break;
            case "explain.findAndModify":
                if ( CommLib.compareVersion(
                        CommLib.getMongoDBVersion( mongoClient ),
                        "3.6" ) >= 0 ) {
                    Assert.assertEquals( cmd.getString( "opCode" ), "OP_MSG" );
                } else {
                    Assert.assertEquals( cmd.getString( "opCode" ),
                            "OP_QUERY" );
                }
                Assert.assertEquals( cmd.getIntValue( "count" ), 1 );
                Assert.assertEquals( cmd.getString( "SequoiaDB" ), "Y" );
                Assert.assertEquals( cmd.getString( "Fap" ), "N" );
                for ( int j = 0; j < parameters.size(); j++ ) {
                    JSONObject parameter = parameters.getJSONObject( j );
                    Assert.assertEquals( parameter.getIntValue( "count" ), 1 );
                    String parameterName = parameter.getString( "param" );
                    switch ( parameterName ) {
                    case "findAndModify.findAndModify":
                    case "findAndModify.query":
                    case "findAndModify.sort":
                    case "findAndModify.new":
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals(
                                    parameter.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals(
                                    parameter.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        break;
                    case "findAndModify.update":
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals(
                                    parameter.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals(
                                    parameter.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        JSONArray operators = parameter
                                .getJSONArray( "operators" );
                        JSONObject operater = operators.getJSONObject( 0 );
                        Assert.assertEquals( operater.getString( "operater" ),
                                "$set" );
                        Assert.assertEquals( operater.getIntValue( "count" ),
                                1 );
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals(
                                    operater.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals(
                                    operater.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }

                        break;
                    case "explain.explain":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    default:
                        continue;
                    }
                }
                break;
            case "explain.aggregate":
                Assert.assertEquals( cmd.getString( "opCode" ), "OP_MSG" );
                Assert.assertEquals( cmd.getIntValue( "count" ), 1 );
                Assert.assertEquals( cmd.getString( "SequoiaDB" ), "Y" );
                Assert.assertEquals( cmd.getString( "Fap" ), "N" );
                for ( int j = 0; j < parameters.size(); j++ ) {
                    JSONObject parameter = parameters.getJSONObject( j );
                    Assert.assertEquals( parameter.getIntValue( "count" ), 1 );
                    String parameterName = parameter.getString( "param" );
                    switch ( parameterName ) {
                    case "explain.verbosity":
                    case "aggregate.cursor":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "N" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    case "aggregate.aggregate":
                    case "explain.explain":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    case "aggregate.pipeline":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        JSONArray operators = parameter
                                .getJSONArray( "operators" );
                        Document group = new Document( "operater", "$group" )
                                .append( "count", 1 ).append( "SequoiaDB", "Y" )
                                .append( "Fap", "N" );
                        Document match = new Document( "operater", "$match" )
                                .append( "count", 1 ).append( "SequoiaDB", "Y" )
                                .append( "Fap", "N" );
                        Document sum = new Document( "operater", "$sum" )
                                .append( "count", 1 ).append( "SequoiaDB", "Y" )
                                .append( "Fap", "N" );
                        Assert.assertTrue( operators.contains(
                                JSONObject.parse( group.toJson() ) ) );
                        Assert.assertTrue( operators.contains(
                                JSONObject.parse( match.toJson() ) ) );
                        Assert.assertTrue( operators
                                .contains( JSONObject.parse( sum.toJson() ) ) );
                        break;
                    default:
                        continue;
                    }
                }
                break;
            case "explain.count":
                if ( CommLib.compareVersion(
                        CommLib.getMongoDBVersion( mongoClient ),
                        "3.6" ) >= 0 ) {
                    Assert.assertEquals( cmd.getString( "opCode" ), "OP_MSG" );
                } else {
                    Assert.assertEquals( cmd.getString( "opCode" ),
                            "OP_QUERY" );
                }
                Assert.assertEquals( cmd.getIntValue( "count" ), 1 );
                Assert.assertEquals( cmd.getString( "SequoiaDB" ), "Y" );
                Assert.assertEquals( cmd.getString( "Fap" ), "N" );
                for ( int j = 0; j < parameters.size(); j++ ) {
                    JSONObject parameter = parameters.getJSONObject( j );
                    Assert.assertEquals( parameter.getIntValue( "count" ), 1 );
                    String parameterName = parameter.getString( "param" );
                    switch ( parameterName ) {
                    case "count.count":
                    case "count.skip":
                    case "count.limit":
                    case "count.hint":
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals(
                                    parameter.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals(
                                    parameter.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        break;
                    case "count.query":
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals(
                                    parameter.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals(
                                    parameter.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        JSONArray operators = parameter
                                .getJSONArray( "operators" );
                        JSONObject operater = operators.getJSONObject( 0 );
                        Assert.assertEquals( operater.getString( "operater" ),
                                "$gt" );
                        Assert.assertEquals( operater.getIntValue( "count" ),
                                1 );
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals(
                                    operater.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals(
                                    operater.getString( "SequoiaDB" ), "Y" );
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        break;
                    case "explain.verbose":
                    case "count.readConcern":
                    case "count.maxTimeMS":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "N" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    case "explain.explain":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    default:
                        continue;
                    }
                }
                break;
            case "explain.distinct":
                if ( CommLib.compareVersion(
                        CommLib.getMongoDBVersion( mongoClient ),
                        "3.6" ) >= 0 ) {
                    Assert.assertEquals( cmd.getString( "opCode" ), "OP_MSG" );
                } else {
                    Assert.assertEquals( cmd.getString( "opCode" ),
                            "OP_QUERY" );
                }
                Assert.assertEquals( cmd.getIntValue( "count" ), 1 );
                Assert.assertEquals( cmd.getString( "SequoiaDB" ), "N" );
                Assert.assertEquals( cmd.getString( "Fap" ), "N" );
                for ( int j = 0; j < parameters.size(); j++ ) {
                    JSONObject parameter = parameters.getJSONObject( j );
                    Assert.assertEquals( parameter.getIntValue( "count" ), 1 );
                    String parameterName = parameter.getString( "param" );
                    switch ( parameterName ) {
                    case "distinct.distinct":
                    case "distinct.key":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "N" );
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {

                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        break;
                    case "distinct.query":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "N" );
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {

                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        JSONArray operators = parameter
                                .getJSONArray( "operators" );
                        JSONObject operater = operators.getJSONObject( 0 );
                        Assert.assertEquals( operater.getString( "operater" ),
                                "$gt" );
                        Assert.assertEquals( operater.getIntValue( "count" ),
                                1 );
                        Assert.assertEquals( operater.getString( "SequoiaDB" ),
                                "N" );
                        Assert.assertEquals( operater.getString( "Fap" ), "N" );
                        break;
                    case "explain.verbose":
                    case "distinct.readConcern":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "N" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    case "explain.explain":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    default:
                        continue;
                    }
                }
                break;
            case "explain.update":
                if ( CommLib.compareVersion(
                        CommLib.getMongoDBVersion( mongoClient ),
                        "3.6" ) >= 0 ) {
                    Assert.assertEquals( cmd.getString( "opCode" ), "OP_MSG" );
                } else {
                    Assert.assertEquals( cmd.getString( "opCode" ),
                            "OP_QUERY" );
                }
                Assert.assertEquals( cmd.getIntValue( "count" ), 1 );
                Assert.assertEquals( cmd.getString( "SequoiaDB" ), "Y" );
                Assert.assertEquals( cmd.getString( "Fap" ), "N" );
                for ( int j = 0; j < parameters.size(); j++ ) {
                    JSONObject parameter = parameters.getJSONObject( j );
                    Assert.assertEquals( parameter.getIntValue( "count" ), 1 );
                    String parameterName = parameter.getString( "param" );
                    switch ( parameterName ) {
                    case "update.update":
                    case "update.updates.q":
                    case "update.updates":
                    case "update.updates.multi":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        break;
                    case "update.maxTimeMS":
                    case "update.ordered":
                    case "update.byPassDocumentValidation":
                    case "explain.verbose":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "N" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    case "explain.explain":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    case "update.updates.u":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        JSONArray operators = parameter
                                .getJSONArray( "operators" );
                        JSONObject operater = operators.getJSONObject( 0 );
                        Assert.assertEquals( operater.getString( "operater" ),
                                "$set" );
                        Assert.assertEquals( operater.getIntValue( "count" ),
                                1 );
                        Assert.assertEquals( operater.getString( "SequoiaDB" ),
                                "Y" );
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals( operater.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals( operater.getString( "Fap" ),
                                    "N" );
                        }
                        break;
                    default:
                        continue;
                    }
                }
                break;
            case "explain.delete":
                if ( CommLib.compareVersion(
                        CommLib.getMongoDBVersion( mongoClient ),
                        "3.6" ) >= 0 ) {
                    Assert.assertEquals( cmd.getString( "opCode" ), "OP_MSG" );
                } else {
                    Assert.assertEquals( cmd.getString( "opCode" ),
                            "OP_QUERY" );
                }
                Assert.assertEquals( cmd.getIntValue( "count" ), 1 );
                Assert.assertEquals( cmd.getString( "SequoiaDB" ), "Y" );
                Assert.assertEquals( cmd.getString( "Fap" ), "N" );
                for ( int j = 0; j < parameters.size(); j++ ) {
                    JSONObject parameter = parameters.getJSONObject( j );
                    Assert.assertEquals( parameter.getIntValue( "count" ), 1 );
                    String parameterName = parameter.getString( "param" );
                    switch ( parameterName ) {
                    case "delete.deletes.q":
                    case "delete.deletes":
                    case "delete.delete":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        if ( cmd.getString( "opCode" ).equals( "OP_QUERY" ) ) {
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "Y" );
                        } else {
                            Assert.assertEquals( parameter.getString( "Fap" ),
                                    "N" );
                        }
                        break;
                    case "delete.maxTimeMS":
                    case "delete.deletes.limit":
                    case "delete.ordered":
                    case "explain.verbose":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "N" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    case "explain.explain":
                        Assert.assertEquals( parameter.getString( "SequoiaDB" ),
                                "Y" );
                        Assert.assertEquals( parameter.getString( "Fap" ),
                                "N" );
                        break;
                    default:
                        continue;
                    }
                }
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
