package com.mongodb.m2s.collector;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-33037:收集集群信息，自动跳过系统数据库
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/31
 * @UpdateRemark
 * @Version
 */
public class Collector_33037 extends M2STestBase {
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
        CommLib.collectCollection( ssh, collectSample );

        // 检验集群信息
        ssh.exec( "cat " + collectorOutputPath + "cluster.json" );
        JSONObject cluster = JSONObject.parseObject( ssh.getStdout() );
        // 校验每个数据库的信息
        JSONArray databases = cluster.getJSONArray( "databases" );
        for ( int i = 0; i < databases.size(); i++ ) {
            JSONObject database = databases.getJSONObject( i );
            String databaseName = database.getString( "name" );
            if ( databaseName.equals( "admin" )
                    || databaseName.equals( "config" )
                    || databaseName.equals( "local" ) ) {
                Assert.fail(
                        "expected not to collect system database but collected" );
            }
        }
        // 检验集合信息
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] split = ssh.getStdout().split( "\n" );
        for ( int i = 0; i < split.length; i++ ) {
            JSONObject collection = JSONObject.parseObject( split[ i ] );
            String databaseName = collection.getString( "database" );
            if ( databaseName.equals( "admin" )
                    || databaseName.equals( "config" )
                    || databaseName.equals( "local" ) ) {
                Assert.fail(
                        "expected not to collect system database but collected" );
            }
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
