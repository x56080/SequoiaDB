package com.mongodb.dds_ctl;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33653:指定--configsvr，--shardsvr部署分片模式集群
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/23
 * @UpdateRemark
 * @Version
 */
public class CTL_33653 extends CTLTestBase {
    private Ssh ssh = null;
    private Integer port1 = 10000;
    private Integer port2 = 11000;
    private Integer port3 = 12000;
    private Integer port4 = 13000;
    private Integer port5 = 14000;
    private Integer port6 = 15000;
    private Integer port7 = 16000;
    private Integer port8 = 17000;
    private String replnameConfig = "rs";
    private String replnameShard1 = "shard1";
    private String replnameShard2 = "shard2";
    private String replnameShard3 = "shard3";
    private String replnameShard4 = "shard4";
    private MongoClient replicaSetClient = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 指定--replname部署分片模式
        ssh.exec( ctlPath + " initdb --port " + port1 + " --replname "
                + replnameConfig + " --configsvr " + " --dbpath " + dataBasePath
                + port1 );
        ssh.exec( ctlPath + " initdb --port " + port2 + " --replname "
                + replnameConfig + " --configsvr " + " --dbpath " + dataBasePath
                + port2 );
        ssh.exec( ctlPath + " initdb --port " + port3 + " --replname "
                + replnameConfig + " --configsvr " + " --dbpath " + dataBasePath
                + port3 );
        ssh.exec( ctlPath + " initdb --port " + port4 + " --replname "
                + replnameShard1 + " --shardsvr " + " --dbpath " + dataBasePath
                + port4 );
        ssh.exec( ctlPath + " initdb --port " + port5 + " --replname "
                + replnameShard2 + " --shardsvr " + " --dbpath " + dataBasePath
                + port5 );
        ssh.exec( ctlPath + " initdb --port " + port6 + " --replname "
                + replnameShard3 + " --shardsvr " + " --dbpath " + dataBasePath
                + port6 );
        ssh.exec( ctlPath + " initrt --port " + port7 + " --dbpath "
                + dataBasePath + port7 + " --configdb " + replnameConfig + "/"
                + remoteHost + ":" + port1 + "," + remoteHost + ":" + port2
                + "," + remoteHost + ":" + port3 );
        ssh.exec( ctlPath + " initdb --port " + port8 + " --replname "
                + replnameShard4 + " --shardsvr " + " --dbpath " + dataBasePath
                + port8 );

        // sdb_dds_ctl工具启动节点
        ssh.exec( ctlPath + " start --all" );

        // 初始化分片集群配置节点
        ssh.exec( mongoshPath + " --port " + port1 + " --eval=\""
                + replnameConfig + ".initiate({_id:'" + replnameConfig
                + "',version:1,members:[{_id:0,host:'" + remoteHost + ":"
                + port1 + "',priority: 2},{_id:1,host:'" + remoteHost + ":"
                + port2 + "'},{_id:3,host:'" + remoteHost + ":" + port3
                + "'}]})\"" );
        // 初始化分片集群数据节点
        ssh.exec( mongoshPath + " --port " + port4
                + " --eval=\"rs.initiate({_id:'" + replnameShard1
                + "',version:1,members:[{_id:0,host:'" + remoteHost + ":"
                + port4 + "'}]})\"" );
        CommLib.checkCluster( port4 );
        ssh.exec( mongoshPath + " --port " + port5
                + " --eval=\"rs.initiate({_id:'" + replnameShard2
                + "',version:1,members:[{_id:0,host:'" + remoteHost + ":"
                + port5 + "'}]})\"" );
        CommLib.checkCluster( port5 );
        ssh.exec( mongoshPath + " --port " + port6
                + " --eval=\"rs.initiate({_id:'" + replnameShard3
                + "',version:1,members:[{_id:0,host:'" + remoteHost + ":"
                + port6 + "'}]})\"" );
        CommLib.checkCluster( port6 );

        // 初始化分片集群路由节点
        ssh.exec( mongoshPath + " --port " + port7 + " --eval=\"sh.addShard('"
                + replnameShard1 + "/" + remoteHost + ":" + port4 + "')\"" );
        ssh.exec( mongoshPath + " --port " + port7 + " --eval=\"sh.addShard('"
                + replnameShard2 + "/" + remoteHost + ":" + port5 + "')\"" );
        ssh.exec( mongoshPath + " --port " + port7 + " --eval=\"sh.addShard('"
                + replnameShard3 + "/" + remoteHost + ":" + port6 + "')\"" );

        // 移除分片，执行基本数据操作
        replicaSetClient = MongoClients
                .create( "mongodb://" + remoteHost + ":" + port7 );
        CommLib.checkShardRemove( replicaSetClient, replnameShard3 );
        CommLib.crudDDS( replicaSetClient );

        // 添加分片，执行基本数据操作
        ssh.exec( mongoshPath + " --port " + port8
                + " --eval=\"rs.initiate({_id:'" + replnameShard4
                + "',version:1,members:[{_id:0,host:'" + remoteHost + ":"
                + port8 + "'}]})\"" );
        CommLib.checkCluster( port8 );
        ssh.exec( mongoshPath + " --port " + port7 + " --eval=\"sh.addShard('"
                + replnameShard4 + "/" + remoteHost + ":" + port8 + "')\"" );
        CommLib.crudDDS( replicaSetClient );
    }

    @AfterClass
    public void teardown() throws Exception {
        if ( replicaSetClient != null ) {
            replicaSetClient.close();
        }
        CommLib.deleteNode( ssh );
        ssh.disconnect();
    }
}
