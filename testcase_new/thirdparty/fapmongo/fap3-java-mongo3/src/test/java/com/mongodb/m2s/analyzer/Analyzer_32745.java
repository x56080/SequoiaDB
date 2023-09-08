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
        analyzeWithSDBVersion( "7.0.0", null );
        analyzeWithSDBVersion( "3.6", null );

        // 测试--sdbversion/-s无效值
        String expectError;
        expectError = "error: missing parameter: sdbversion" + "\n";
        analyzeWithSDBVersion( null, expectError );

        expectError = "error: invalid version format: -7.0" + "\n";
        analyzeWithSDBVersion( "-7.0", expectError );

        expectError = "error: invalid version format: hello" + "\n";
        analyzeWithSDBVersion( "hello", expectError );

        expectError = "error: invalid version format: null" + "\n";
        analyzeWithSDBVersion( "null", expectError );

        expectError = "error: invalid version format: 7.-1.0" + "\n";
        analyzeWithSDBVersion( "7.-1.0", expectError );

        expectError = "error: invalid version format: 7.0.0.0" + "\n";
        analyzeWithSDBVersion( "7.0.0.0", expectError );

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, analyzerOutputPath );
        if ( mongoClient != null )
            mongoClient.close();
        if ( ssh != null )
            ssh.disconnect();
    }

    private void analyzeWithSDBVersion( String sdbVersion, String errorMsg )
            throws Exception {
        String analyzeCmd = analyzerPath + " -o " + analyzerOutputPath;
        if ( sdbVersion != null ) {
            analyzeCmd = analyzeCmd + " -s " + sdbVersion;
        }
        if ( errorMsg == null ) {
            ssh.exec( analyzeCmd );
            ssh.exec( "cat " + analyzerOutputPath + "summary.json" );
            JSONObject summaryJson = JSONObject.parseObject( ssh.getStdout() );
            Assert.assertTrue(
                    sdbVersion.equals( summaryJson.getString( "SequoiaDB" ) ) );
        } else {
            try {
                ssh.exec( analyzeCmd );
                Assert.fail( "expect error but success" );
            } catch ( Exception e ) {
                Assert.assertTrue( errorMsg.equals( ssh.getStderr() ) );
            }
        }
    }
}
