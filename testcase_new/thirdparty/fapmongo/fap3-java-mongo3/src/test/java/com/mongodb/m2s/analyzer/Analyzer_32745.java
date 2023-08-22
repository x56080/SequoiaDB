package com.mongodb.m2s.analyzer;

import org.testng.Assert;
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
 * @Descreption seqDB-32745:sdbversion/-s参数校验
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32745 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, analyzerOutputPath );
    }

    @Test
    public void test() throws Exception {
        // 指定为正确的版本格式（包含一个或两个小数点），包含7.0以上版本和7.0以下版本
        ssh.exec( analyzerPath + " -s 7.0.0 -o " + analyzerOutputPath );
        ssh.exec( "cat " + analyzerOutputPath + "summary.json" );
        JSONObject summaryJson1 = JSONObject.parseObject( ssh.getStdout() );
        Assert.assertTrue(
                "7.0.0".equals( summaryJson1.getString( "SequoiaDB" ) ) );
        ssh.exec( analyzerPath + " --sdbversion 3.6 -o " + analyzerOutputPath );
        ssh.exec( "cat " + analyzerOutputPath + "summary.json" );
        JSONObject summaryJson2 = JSONObject.parseObject( ssh.getStdout() );
        Assert.assertTrue(
                "3.6".equals( summaryJson2.getString( "SequoiaDB" ) ) );

        // 测试--sdbversion/-s无效值
        String expectError;
        try {
            ssh.exec( analyzerPath );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            expectError = "error: missing parameter: sdbversion" + "\n";
            Assert.assertTrue( expectError.equals( ssh.getStderr() ) );
        }
        try {
            ssh.exec( analyzerPath + " -s -7.0 -o " + analyzerOutputPath );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            expectError = "error: invalid version format: -7.0" + "\n";
            Assert.assertTrue( expectError.equals( ssh.getStderr() ) );
        }
        try {
            ssh.exec( analyzerPath + " -s hello -o " + analyzerOutputPath );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            expectError = "error: invalid version format: hello" + "\n";
            Assert.assertTrue( expectError.equals( ssh.getStderr() ) );
        }
        try {
            ssh.exec( analyzerPath + " -s null -o " + analyzerOutputPath );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            expectError = "error: invalid version format: null" + "\n";
            Assert.assertTrue( expectError.equals( ssh.getStderr() ) );
        }
        try {
            ssh.exec( analyzerPath + " -s 7.-1.0 -o " + analyzerOutputPath );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            expectError = "error: invalid version format: 7.-1.0" + "\n";
            Assert.assertTrue( expectError.equals( ssh.getStderr() ) );
        }
        try {
            ssh.exec( analyzerPath + " -s 7.0.0.0 -o " + analyzerOutputPath );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            expectError = "error: invalid version format: 7.0.0.0" + "\n";
            Assert.assertTrue( expectError.equals( ssh.getStderr() ) );
        }
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, analyzerOutputPath );
        if ( mongoClient != null )
            mongoClient.close();
        if ( ssh != null )
            ssh.disconnect();
    }

}
