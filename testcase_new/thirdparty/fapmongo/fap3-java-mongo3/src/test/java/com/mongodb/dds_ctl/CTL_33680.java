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
 * @Descreption seqDB-33680:分别在 arm 和 x86 机器上部署分片集群
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/20
 * @UpdateRemark
 * @Version
 */
public class CTL_33680 extends CTLTestBase {
    private Ssh ssh = null;
    private MongoClient shardingClient = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
        // 创建配置文件目录
        ssh.exec( "mkdir -p " + configFilePath );
    }

    @Test
    public void test() throws Exception {
        // 搭建三节点分片集群
        CommLib.createSharding( ssh, false );

        // 执行基本数据操作
        shardingClient = MongoClients
                .create( "mongodb://" + remoteHost + ":" + shardingPort );
        CommLib.crudDDS( shardingClient );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        // 删除配置文件目录
        ssh.exec( "rm -rf " + configFilePath );
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
