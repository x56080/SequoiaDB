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
 * @Descreption seqDB-33638:configfile文件指定端口号、数据目录和日志文件（必要参数）
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/18
 * @UpdateRemark
 * @Version
 */
public class CTL_33638 extends CTLTestBase {
    private MongoClient shardingClient = null;
    private MongoClient replicaSetClient = null;
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 搭建三节点分片集群
        CommLib.createSharding( ssh, false );

        // 执行基本数据操作
        shardingClient = MongoClients
                .create( "mongodb://" + remoteHost + ":" + shardingPort );
        CommLib.crudDDS( shardingClient );

        // 清理环境
        shardingClient.close();
        CommLib.deleteNode( ssh );

        // 搭建三节点副本集集群
        CommLib.createReplicaSet( ssh, false );

        // 执行基本数据操作
        replicaSetClient = MongoClients
                .create( "mongodb://" + remoteHost + ":" + replicaSetPort );
        CommLib.crudDDS( replicaSetClient );
    }

    @AfterClass
    public void teardown() throws Exception {
        if ( shardingClient != null ) {
            shardingClient.close();
        }
        if ( replicaSetClient != null ) {
            replicaSetClient.close();
        }
        CommLib.deleteNode( ssh );
        ssh.disconnect();
    }
}
