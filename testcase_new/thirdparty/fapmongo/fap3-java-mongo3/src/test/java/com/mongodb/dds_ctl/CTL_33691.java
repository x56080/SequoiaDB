package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33691:指定--force不指定--port或--all
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33691 extends CTLTestBase {
    private Ssh ssh = null;
    private Integer port1 = 10000;
    private Integer port2 = 11000;
    private Integer port3 = 12000;
    private String replname = "rs";

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 创建多个节点
        ssh.exec( ctlPath + " initdb --port " + port1 + " --replname "
                + replname + " --dbpath " + dataBasePath + port1 );
        ssh.exec( ctlPath + " initdb --port " + port2 + " --replname "
                + replname + " --dbpath " + dataBasePath + port2 );
        ssh.exec( ctlPath + " initdb --port " + port3 + " --replname "
                + replname + " --dbpath " + dataBasePath + port3 );

        // 启动所有节点
        ssh.exec( ctlPath + " start --all" );

        // 同时指定--all、--port停止节点
        try {
            ssh.exec( ctlPath + " stop --force" );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains(
                    "\"stop\" needs to be used with \"--port\" or \"--all\"" ) ) {
                throw e;
            }
        }

        // 校验端口信息
        ssh.exec( ctlPath + " list --all" );
        String listString = ssh.getStdout();
        CommLib.checkNode( listString, "cluster_role", "replset" );
        CommLib.checkNode( listString, "status", "running" );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        ssh.disconnect();
    }
}
