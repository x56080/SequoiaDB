package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33674:action 和 options 不匹配
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/19
 * @UpdateRemark
 * @Version
 */
public class CTL_33674 extends CTLTestBase {
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 初始化节点
        ssh.exec( ctlPath
                + " initdb --all --force --replname=rs0 --port=10000 --configsvr --dbpath=/opt/sequoiadds/database/10000" );
        // 启动节点
        ssh.exec( ctlPath
                + " start --port=10000 --bindip=192.168.17.85 --configsvr --dbpath=/opt/sequoiadds/database/10000" );
        // 检查节点状态
        ssh.exec( ctlPath + " list --port=10000" );
        CommLib.checkNode( ssh.getStdout(), "status", "running" );
        // 同时指定-v\-h参数
        ssh.exec( ctlPath + " -v -h" );
        // 检查执行结果，-v优先生效
        Assert.assertTrue( ssh.getStdout().contains( "version" ) );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
