package com.mongodb.m2s.analyzer;

import java.util.*;

import com.alibaba.fastjson.JSONArray;
import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32908:执行aggregate语句，分析消息报告
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/28
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32908 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32908";
    private String collectionName = "col_32908";

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
                            .append( "c", Arrays.asList( i, i * 2 ) )
                            .append( "d", i + 2 )
                            .append( "date", new Date() ) );
        }
        // 启动sniffer工具
        CommLib.snifferStart( ssh );
        // 连接sniffer监听端口
        MongoClient snifferClient = CommLib.getSnifferClient();

        String outCL = "out_32908";
        snifferClient.getDatabase( databaseName ).createCollection( outCL );
        MongoCollection< Document > cl = snifferClient
                .getDatabase( databaseName ).getCollection( collectionName );
        ArrayList< Document > pipeline = new ArrayList<>();
        // $sample
        Document sample = new Document( "$sample", new Document( "size", 30 ) );
        pipeline.add( sample );
        // $match
        Document match = new Document( "$match",
                new Document( "or",
                        Arrays.asList(
                                new Document( "b",
                                        new Document( "gt", 10 ).append( "lt",
                                                20 ) ),
                                new Document( "b", new Document( "gte", 30 )
                                        .append( "lte", 40 ) ) ) ) );
        pipeline.add( match );
        // $group
        Document group = new Document( "$group", new Document( "_id", "$b" )
                .append( "count", new Document( "$sum", 1 ) ) );
        pipeline.add( group );
        // $sort
        Document sort = new Document( "$sort",
                new Document( "count", -1 ).append( "_id", 1 ) );
        pipeline.add( sort );
        // $limit
        Document limit = new Document( "$limit", 10 );
        pipeline.add( limit );
        // $project
        Document project = new Document( "$project", new Document( "_id", 0 )
                .append( "b", "$_id" ).append( "count", 1 )
                .append( "abs",
                        new Document( "$abs",
                                new Document( "$subtract",
                                        Arrays.asList( "$b", "$d" ) ) ) )
                .append( "cmp",
                        new Document( "$cmp", Arrays.asList( "$b", 15 ) ) )
                .append( "substr",
                        new Document( "$substr", Arrays.asList( "$a", 1, 3 ) ) )
                .append( "hour", new Document( "$hour", "$date" ) )
                .append( "max",
                        new Document( "$max", Arrays.asList( "$b", "$d" ) ) ) );
        pipeline.add( project );
        // $unwind
        Document unwind = new Document( "$unwind", "$c" );
        pipeline.add( unwind );
        // $out
        Document out = new Document( "$out", outCL );
        pipeline.add( out );

        cl.aggregate( pipeline ).allowDiskUse( true ).into( new ArrayList<>() );

        CommLib.snifferStopAndAnalyze( ssh );

        CommLib.analyzeSnifferMsgJson( ssh );
        ssh.exec( "cat " + analyzerOutputPath + "sniffer.json" );

        JSONArray cmds = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < cmds.size(); i++ ) {
            JSONObject cmd = cmds.getJSONObject( i );
            if ( cmd.getString( "databaseCmd" ).equals( "aggregate" ) ) {
                Assert.assertEquals( cmd.getIntValue( "count" ), 1 );
                if ( CommLib.compareVersion(
                        CommLib.getMongoDBVersion( mongoClient ),
                        "3.6" ) >= 0 ) {
                    Assert.assertEquals( cmd.getString( "opCode" ), "OP_MSG" );
                    Assert.assertEquals( cmd.getString( "SequoiaDB" ), "Y" );
                    Assert.assertEquals( cmd.getString( "Fap" ), "N" );
                } else {
                    Assert.assertEquals( cmd.getString( "opCode" ),
                            "OP_QUERY" );
                    Assert.assertEquals( cmd.getString( "SequoiaDB" ), "Y" );
                    Assert.assertEquals( cmd.getString( "Fap" ), "Y" );
                }
                JSONArray parameters = cmd.getJSONArray( "parameters" );
                for ( int j = 0; j < parameters.size(); j++ ) {
                    JSONObject parameter = parameters.getJSONObject( j );
                    String paramName = parameter.getString( "param" );
                    switch ( paramName ) {
                    case "pipeline":
                        Assert.assertEquals( 1,
                                parameter.getInteger( "count" ).intValue() );
                        Assert.assertEquals( "Y",
                                parameter.getString( "SequoiaDB" ) );
                        if ( CommLib.compareVersion(
                                CommLib.getMongoDBVersion( mongoClient ),
                                "3.6" ) >= 0 ) {
                            Assert.assertEquals( "N",
                                    parameter.getString( "Fap" ) );
                        } else {
                            Assert.assertEquals( "Y",
                                    parameter.getString( "Fap" ) );
                        }
                        JSONArray operators = parameter
                                .getJSONArray( "operators" );
                        for ( int k = 0; k < operators.size(); k++ ) {
                            JSONObject operator = operators.getJSONObject( k );
                            String operaName = operator.getString( "operater" );
                            switch ( operaName ) {
                            case "$sort":
                            case "$limit":
                            case "$sum":
                            case "$unwind":
                            case "$project":
                            case "$match":
                            case "$max":
                            case "$group":
                                Assert.assertEquals( 1, operator
                                        .getInteger( "count" ).intValue() );
                                Assert.assertEquals( "Y",
                                        operator.getString( "SequoiaDB" ) );
                                if ( CommLib
                                        .compareVersion(
                                                CommLib.getMongoDBVersion(
                                                        mongoClient ),
                                                "3.6" ) >= 0 ) {
                                    Assert.assertEquals( "N",
                                            operator.getString( "Fap" ) );
                                } else {
                                    Assert.assertEquals( "Y",
                                            operator.getString( "Fap" ) );
                                }
                                break;
                            case "$out":
                            case "$sample":
                            case "$cmp":
                            case "$hour":
                            case "$substr":
                            case "$abs":
                            case "$subtract":
                                Assert.assertEquals( 1, operator
                                        .getInteger( "count" ).intValue() );
                                Assert.assertEquals( "N",
                                        operator.getString( "SequoiaDB" ) );
                                Assert.assertEquals( "N",
                                        operator.getString( "Fap" ) );
                                break;

                            }

                        }
                        break;
                    case "cursor":
                    case "allowDiskUse":
                        Assert.assertEquals( 1,
                                parameter.getInteger( "count" ).intValue() );
                        Assert.assertEquals( "N",
                                parameter.getString( "SequoiaDB" ) );

                        Assert.assertEquals( "N",
                                parameter.getString( "Fap" ) );

                        break;
                    case "aggregate":
                        Assert.assertEquals( 1,
                                parameter.getInteger( "count" ).intValue() );
                        Assert.assertEquals( "Y",
                                parameter.getString( "SequoiaDB" ) );
                        if ( CommLib.compareVersion(
                                CommLib.getMongoDBVersion( mongoClient ),
                                "3.6" ) >= 0 ) {
                            Assert.assertEquals( "N",
                                    parameter.getString( "Fap" ) );
                        } else {
                            Assert.assertEquals( "Y",
                                    parameter.getString( "Fap" ) );
                        }
                        break;
                    default:
                        continue;
                    }
                }
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
