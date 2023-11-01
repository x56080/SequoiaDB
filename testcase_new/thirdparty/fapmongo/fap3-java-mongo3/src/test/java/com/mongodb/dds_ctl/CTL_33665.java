package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33665:创建节点不指定--bindip
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33665 extends CTLTestBase {
    private Ssh ssh = null;
    private Integer port = 10000;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 创建节点
        ssh.exec( ctlPath + " initdb --port " + port + " --dbpath "
                + dataBasePath + port );

        // sdb_dds_ctl工具启动节点
        ssh.exec( ctlPath + " start --all" );

        // 校验端口信息
        ssh.exec( ctlPath + " list --port " + port );
        String listString = ssh.getStdout();
        CommLib.checkNode( listString, "cluster_role", "standalone" );
        CommLib.checkNode( listString, "port", "10000" );

        // 查看conf/local/10000.yaml文件,校验bindip
        ssh.exec( "cat " + confPath + port + ".yaml" );
        String stdout = ssh.getStdout();
        Assert.assertTrue( stdout.contains( "bindIp: 0.0.0.0" ) );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        ssh.disconnect();
    }
}
