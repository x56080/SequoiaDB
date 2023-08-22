package com.mongodb.m2s.collector;

import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-32655:工具收集不同部署模式的集群信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Collector_32655 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
    }

    @Test
    public void test() throws Exception {
        CommLib.collectCluster( ssh );
        ssh.exec( "cat " + collectorOutputPath + "cluster.json" );
        JSONObject cluster = JSONObject.parseObject( ssh.getStdout() );
        if ( CommLib.isStandalone( mongoClient ) ) {
            Assert.assertEquals( cluster.getString( "instMode" ),
                    "standalone" );
            Assert.assertNotNull( cluster.get( "standalone" ) );
            Assert.assertNull( cluster.get( "replset" ) );
            Assert.assertNull( cluster.get( "shards" ) );
        } else if ( CommLib.isReplicaSet( mongoClient ) ) {
            Assert.assertEquals( cluster.getString( "instMode" ), "replset" );
            Assert.assertNull( cluster.get( "standalone" ) );
            Assert.assertNotNull( cluster.get( "replset" ) );
            Assert.assertNull( cluster.get( "shards" ) );
        } else if ( CommLib.isSharded( mongoClient ) ) {
            Assert.assertEquals( cluster.getString( "instMode" ), "shards" );
            Assert.assertNull( cluster.get( "standalone" ) );
            Assert.assertNull( cluster.get( "replset" ) );
            Assert.assertNotNull( cluster.get( "shards" ) );
        } else {
            Assert.fail( "Unknown deployment mode" );
        }

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        if ( mongoClient != null ) {
            mongoClient.close();
        }
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
