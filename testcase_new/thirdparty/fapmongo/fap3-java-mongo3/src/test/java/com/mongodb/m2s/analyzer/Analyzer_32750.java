package com.mongodb.m2s.analyzer;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32750:outputtype/-t参数校验
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32750 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
    }

    @Test
    public void test() throws Exception {
        // 测试--outputtype/-t参数有效值
        analyzeWithType( null );
        ssh.exec( "ls " + analyzerOutputPath );
        Assert.assertTrue( ssh.getStdout().contains( "summary.json" ) );
        Assert.assertFalse(
                ssh.getStdout().contains( "m2s-analyze-report.xlsx" ) );

        analyzeWithType( "json" );
        ssh.exec( "ls " + analyzerOutputPath );
        Assert.assertTrue( ssh.getStdout().contains( "summary.json" ) );
        Assert.assertFalse(
                ssh.getStdout().contains( "m2s-analyze-report.xlsx" ) );

        analyzeWithType( "excel" );
        ssh.exec( "ls " + analyzerOutputPath );
        Assert.assertFalse( ssh.getStdout().contains( "summary.json" ) );
        Assert.assertTrue(
                ssh.getStdout().contains( "m2s-analyze-report.xlsx" ) );

        analyzeWithType( "json,excel" );
        ssh.exec( "ls " + analyzerOutputPath );
        Assert.assertTrue( ssh.getStdout().contains( "summary.json" ) );
        Assert.assertTrue(
                ssh.getStdout().contains( "m2s-analyze-report.xlsx" ) );

        // 测试--outputtype/-t无效值
        try {
            analyzeWithType( "txt" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "error: invalid output type: txt" + "\n";
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

    private void analyzeWithType( String type ) throws Exception {
        CommLib.initDir( ssh, analyzerOutputPath );
        String analyzeCmd = analyzerPath + " -s " + sdbVersion + " -o "
                + analyzerOutputPath;
        if ( type != null ) {
            analyzeCmd += " -t " + type;
        }
        ssh.exec( analyzeCmd );
    }

}
