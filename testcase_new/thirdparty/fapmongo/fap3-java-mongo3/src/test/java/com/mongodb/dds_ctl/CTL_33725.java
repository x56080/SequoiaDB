package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33725:创建节点指定--dbpath的目录为绝对路径
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33725 extends CTLTestBase {
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 初始化节点, 指定--dbpath的目录为绝对路径
        ssh.exec( ctlPath
                + " initdb --replname=rs0 --port=10000 --configsvr --dbpath=/opt/sequoiadds/database/10000" );
        // 启动节点
        ssh.exec( ctlPath + " start --all" );
        // 校验节点的dbpath信息
        ssh.exec( ctlPath + " list --port=10000" );
        CommLib.checkNode( ssh.getStdout(), "dbpath", "/opt/sequoiadds/database/10000" );
        // 校验dbpath目录已创建，目录所属人为sdbadmin
        ssh.exec( "ls -ld /opt/sequoiadds/database/10000" );
        Assert.assertTrue( ssh.getStdout().contains( "sdbadmin" ) );
        // 校验数据文件及日志文件在该目录下
        ssh.exec( "ls -l /opt/sequoiadds/database/10000" );
        Assert.assertTrue( ssh.getStdout().contains( "mongod.log" ) );
        Assert.assertTrue( ssh.getStdout().contains( "mongod.lock" ) );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
