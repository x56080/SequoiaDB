package com.mongodb.m2s.analyzer;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import java.util.Collections;

import com.alibaba.fastjson.JSONArray;
import com.mongodb.m2s.testcommon.DataBaseCmd;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32765:收集op_query类型消息，分析消息报告文件
 *              seqDB-32766:收集op_msg类型消息，分析消息报告文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/29
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32765_32766 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    String deployMode = null;

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        if ( CommLib.isSharded( mongoClient ) ) {
            deployMode = "shards";
        } else if ( CommLib.isReplicaSet( mongoClient ) ) {
            deployMode = "replicaSet";
        } else if ( CommLib.isStandalone( mongoClient ) ) {
            deployMode = "standalone";
        } else {
            Assert.fail( "unknown deploy mode" );
        }
        String version = CommLib.getMongoDBVersion( mongoClient );
        if ( CommLib.compareVersion( version, "3.4" ) >= 0
                && CommLib.compareVersion( version, "4.4" ) < 0 ) {
            throw new SkipException(
                    "skip this test case, this testCase is for 3.2 or 4.4 mongodb version" );
        }
        CommLib.initDir( ssh, snifferOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
    }

    @Test
    public void test() throws Exception {
        // 启动sniffer工具
        CommLib.snifferStart( ssh );

        String snifferUri = null;
        if ( mongodbUri.contains( "@" ) ) {
            snifferUri = mongodbUri.substring( 0,
                    mongodbUri.indexOf( "@" ) + 1 ) + remoteHost + ":"
                    + snifferListenPort;
        } else {
            snifferUri = "mongodb://" + remoteHost + ":" + snifferListenPort;
        }
        // 执行数据库命令
        DataBaseCmd dataBaseCmd = new DataBaseCmd( snifferUri );
        dataBaseCmd.runAllCommands();

        // 停止sniffer工具并分析消息日志文件
        CommLib.snifferStopAndAnalyze( ssh );
        ssh.exec( "cat " + snifferOutputPath + "*.json" );

        CommLib.analyzeSnifferMsgJson( ssh );
        ssh.exec( "cat " + analyzerOutputPath + "sniffer.json" );
        JSONArray messages = JSONObject.parseArray( ssh.getStdout() );
        // 按照opCode和databaseCmd排序, 使每次执行用例的输出结果顺序一致
        Collections.sort( messages, ( o1, o2 ) -> {
            String name1 = ( ( JSONObject ) o1 ).getString( "opCode" );
            String name2 = ( ( JSONObject ) o2 ).getString( "opCode" );
            int opCodeCmp = name1.compareTo( name2 );

            if ( opCodeCmp != 0 ) {
                return opCodeCmp;
            } else {
                String cmd1 = ( ( JSONObject ) o1 ).getString( "databaseCmd" );
                String cmd2 = ( ( JSONObject ) o2 ).getString( "databaseCmd" );
                return cmd1.compareTo( cmd2 );
            }
        } );

        String filePath;
        if ( CommLib.compareVersion( CommLib.getMongoDBVersion( mongoClient ),
                "3.6" ) >= 0 ) {
            filePath = "src/test/java/com/mongodb/m2s/analyzer/op_msg_sniffer.json";
        } else {
            filePath = "src/test/java/com/mongodb/m2s/analyzer/op_query_sniffer.json";
        }
        Path path = Paths.get( filePath );
        String jsonContent = new String( Files.readAllBytes( path ) );
        JSONObject jsonObject = JSONObject.parseObject( jsonContent );
        JSONArray jsonArray = jsonObject.getJSONArray( deployMode );
        // 开启鉴权情况下，会多出数据库命令saslStart和saslContinue
        if ( !mongodbUri.contains( "@" ) ) {
            for ( int i = 0; i < jsonArray.size(); i++ ) {
                JSONObject jsonObject1 = jsonArray.getJSONObject( i );
                if ( jsonObject1.getString( "databaseCmd" )
                        .equals( "saslContinue" )
                        || jsonObject1.getString( "databaseCmd" )
                                .equals( "saslStart" ) ) {
                    jsonArray.remove( i );
                    i--;
                }
            }
        }
        compareJsonArray( messages, jsonArray );

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, snifferOutputPath );
        CommLib.rmDir( ssh, analyzerOutputPath );
        if ( mongoClient != null )
            mongoClient.close();
        if ( ssh != null )
            ssh.disconnect();
    }

    private void compareJsonArray( JSONArray actual, JSONArray expected ) {
        if ( actual.size() != expected.size() ) {
            Assert.fail( "sniffer count is not equal!" );
        }
        for ( int i = 0; i < actual.size(); i++ ) {
            JSONObject jsonObject1 = actual.getJSONObject( i );
            JSONObject jsonObject2 = expected.getJSONObject( i );
            String cmdName = jsonObject1.getString( "databaseCmd" );
            if ( cmdName.equals( "saslContinue" )
                    || cmdName.equals( "saslStart" ) ) {
                // continue;
            }
            // 批量跑用例时“isMaster”,"buildinfo","getlasterror"命令的返回结果字段“count”可能不一致，所以单独比较
            if ( cmdName.equals( "isMaster" ) || cmdName.equals( "buildinfo" )
                    || cmdName.equals( "getlasterror" ) ) {
                if ( cmdName.equals( "getlasterror" ) ) {
                    Assert.assertEquals( jsonObject1.getString( "SequoiaDB" ),
                            "Y" );
                } else {
                    Assert.assertEquals( jsonObject1.getString( "SequoiaDB" ),
                            "N" );
                }
                if ( jsonObject1.getString( "opCode" ).equals( "OP_MSG" ) ) {
                    Assert.assertEquals( jsonObject1.getString( "Fap" ), "N" );
                } else {
                    Assert.assertEquals( jsonObject1.getString( "Fap" ), "Y" );
                }
                continue;
            }
            if ( !jsonObject1.toJSONString()
                    .equals( jsonObject2.toJSONString() ) ) {
                Assert.fail( "expected:" + jsonObject1.toJSONString()
                        + "\nactual:" + jsonObject2.toJSONString() );
            }
        }
    }
}
