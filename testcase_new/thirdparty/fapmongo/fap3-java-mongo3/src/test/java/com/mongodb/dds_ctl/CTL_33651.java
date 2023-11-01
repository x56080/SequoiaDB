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
 * @Descreption seqDB-33651:指定--replname部署副本集模式
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/23
 * @UpdateRemark
 * @Version
 */
public class CTL_33651 extends CTLTestBase {
    private Ssh ssh = null;
    private Integer port1 = 10000;
    private Integer port2 = 11000;
    private Integer port3 = 12000;
    private Integer port4 = 13000;
    private String replname = "rs";
    private MongoClient replicaSetClient = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 指定--replname部署副本集模式
        ssh.exec( ctlPath + " initdb --port " + port1 + " --replname "
                + replname + " --dbpath " + dataBasePath + port1 );
        ssh.exec( ctlPath + " initdb --port " + port2 + " --replname "
                + replname + " --dbpath " + dataBasePath + port2 );
        ssh.exec( ctlPath + " initdb --port " + port3 + " --replname "
                + replname + " --dbpath " + dataBasePath + port3 );
        ssh.exec( ctlPath + " initdb --port " + port4 + " --replname "
                + replname + " --dbpath " + dataBasePath + port4 );

        // sdb_dds_ctl工具启动节点
        ssh.exec( ctlPath + " start --all" );

        // 初始化副本集集群
        ssh.exec( mongoshPath + " --port " + port1 + " --eval=\"" + replname
                + ".initiate({_id:'" + replname
                + "',version:1,members:[{_id:0,host:'" + remoteHost + ":"
                + port1 + "',priority: 2},{_id:1,host:'" + remoteHost + ":"
                + port2 + "'},{_id:3,host:'" + remoteHost + ":" + port3
                + "'}]})\"" );

        // 校验端口信息
        ssh.exec( ctlPath + " list --all" );
        CommLib.checkNode( ssh.getStdout(), "cluster_role", "replset" );
        CommLib.checkCluster( port1 );

        // 添加节点
        ssh.exec( mongoshPath + " --port " + port1 + " --eval=\"rs.add('"
                + remoteHost + ":" + port4 + "')\"" );

        // 移除节点
        ssh.exec( mongoshPath + " --port " + port1 + " --eval=\"rs.remove('"
                + remoteHost + ":" + port3 + "')\"" );

        // 连接dds执行基本数据操作
        replicaSetClient = MongoClients
                .create( "mongodb://" + remoteHost + ":" + port1 );
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
