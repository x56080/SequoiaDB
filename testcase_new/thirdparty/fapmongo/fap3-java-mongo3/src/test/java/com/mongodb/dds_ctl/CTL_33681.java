package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33681:部署分片集群到多台机器
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/20
 * @UpdateRemark
 * @Version
 */
public class CTL_33681 extends CTLTestBase {
    private Ssh ssh1 = null;
    private Ssh ssh2 = null;
    private MongoClient shardingClient = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh1 = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh1 );
        // 创建配置文件目录
        ssh1.exec( "mkdir -p " + configFilePath );
        ssh2 = new Ssh( remoteHost1, remoteUser, remotePwd );
        CommLib.deleteNode( ssh2 );
        // 创建配置文件目录
        ssh2.exec( "mkdir -p " + configFilePath );
    }

    @Test
    public void test() throws Exception {
        CommLib.createShardingOnMulServers( ssh1, ssh2 );

        // 执行基本数据操作
        shardingClient = MongoClients
                .create( "mongodb://" + remoteHost + ":" + shardingPort );
        CommLib.crudDDS( shardingClient );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh1 );
        ssh1.exec( "rm -rf " + configFilePath );
        if ( ssh1 != null ) {
            ssh1.disconnect();
        }
        // 删除配置文件目录
        CommLib.deleteNode( ssh2 );
        // 删除配置文件目录
        ssh2.exec( "rm -rf " + configFilePath );
        if ( ssh2 != null ) {
            ssh2.disconnect();
        }
    }
}
