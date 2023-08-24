package com.mongodb.m2s.sniffer;

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
 * @Descreption seqDB-32599:server-start子命令参数校验，无效值校验
 * @Author tangtao
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class Sniffer_32601 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
    }

    @Test
    public void test() throws Exception {

        // test output-type
        try {
            ssh.exec( snifferPath + " analyze -o 123" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "No enum constant";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }
        try {
            ssh.exec( snifferPath + " analyze -o true" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "No enum constant";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }

        // test thread
        try {
            ssh.exec( snifferPath + " analyze -t true" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "For input string: \"true\"";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }
        try {
            ssh.exec( snifferPath + " analyze -t 3.14" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "For input string: \"3.14\"";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
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
