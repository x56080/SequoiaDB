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
public class Sniffer_32599 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
    }

    @Test
    public void test() throws Exception {

        // test listen port
        try {
            ssh.exec( snifferPath + " server-start -l true" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "For input string: \"true\"";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }
        try {
            ssh.exec( snifferPath + " server-start -l 3.1415" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "For input string: \"3.1415\"";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }
        try {
            ssh.exec( snifferPath + " server-start -l -1" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "port out of range(1024-65535):-1";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }
        try {
            ssh.exec( snifferPath + " server-start -l 999999" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "port out of range(1024-65535):999999";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }

        // test mongodb-url
        try {
            ssh.exec( snifferPath
                    + " server-start -m mongodb://localhost:27017" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "invalid mongodb url: mongodb://localhost:27017";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }

        // test record
        try {
            ssh.exec( snifferPath + " server-start -r 123" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "Unknown RecordLevel: 123";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }
        try {
            ssh.exec( snifferPath + " server-start -r true" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "Unknown RecordLevel: true";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }

        // test message-log-capacity
        try {
            ssh.exec( snifferPath + " server-start -s 30" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "msgLogCapacity must end with GB, like 1GB:30";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }
        try {
            ssh.exec( snifferPath + " server-start -s -1GB" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "msgLogCapacity must greater than equals 1GB:-1GB";
            Assert.assertTrue( ssh.getStderr().contains( expectError ) );
        }
        try {
            ssh.exec( snifferPath + " server-start -s true" );
            Assert.fail( "expect error but success" );
        } catch ( Exception e ) {
            String expectError = "msgLogCapacity must end with GB, like 1GB:true";
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
