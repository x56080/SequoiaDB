package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33649:指定--replname部署单节点副本集模式
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/23
 * @UpdateRemark
 * @Version
 */
public class CTL_33649 extends CTLTestBase {
    private Ssh ssh = null;
    private Integer port = 10000;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 指定--replname部署单节点副本集模式
        ssh.exec( ctlPath + " initdb --port " + port
                + " --replname rs --dbpath " + dataBasePath + port );

        // sdb_dds_ctl工具启动节点
        ssh.exec( ctlPath + " start --all" );

        // 校验端口信息
        ssh.exec( ctlPath + " list --port " + port );
        CommLib.checkNode( ssh.getStdout(), "cluster_role", "replset" );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        ssh.disconnect();
    }
}
