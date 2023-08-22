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
 * @Descreption seqDB-32751:output/-o参数校验
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32751 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
    }

    @Test
    public void test() throws Exception {
        // 测试output/-o参数有效值
        // 不指定
        ssh.exec( analyzerPath + " -s " + sdbVersion );
        Assert.assertTrue( ssh.getStdout().contains( "analysis complete" ) );
        // 指定有效路径，路径下不存在分析报告
        CommLib.rmDir( ssh, analyzerOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
        ssh.exec( analyzerPath + " -s " + sdbVersion + " -o "
                + analyzerOutputPath );
        Assert.assertTrue( ssh.getStdout().contains( "analysis complete" ) );
        // 指定有效路径，路径下存在分析报告
        ssh.exec( analyzerPath + " -s " + sdbVersion + " -o "
                + analyzerOutputPath );
        Assert.assertTrue( ssh.getStdout().contains( "analysis complete" ) );

        // 测试output/-o参数无效值
        // 指定不存在的路径
        try {
            ssh.exec( analyzerPath + " -s " + sdbVersion + " -o " + "/ttttt" );
        } catch ( Exception e ) {
            String expectError = "error: output path does not exist: /ttttt"
                    + "\n";
            Assert.assertTrue( expectError.equals( ssh.getStderr() ) );
        }
        // 指定无写权限的路径
        // TODO
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
