package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33657:指定--dbpath的目录已存在但为空
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33657 extends CTLTestBase {
    private Ssh ssh = null;
    private Ssh sdbSsh = null;
    private Integer port = 10000;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        sdbSsh = new Ssh( remoteHost, sdbUser, sdbPwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 提前创建日志、数据目录
        sdbSsh.exec( "mkdir -p " + dataBasePath + port );

        // 创建节点
        ssh.exec( ctlPath + " initdb --port " + port + " --dbpath "
                + dataBasePath + port );

        // sdb_dds_ctl工具启动节点
        ssh.exec( ctlPath + " start --all" );

        // 验证目录是否创建，目录权限
        ssh.exec( "ls -l " + dataBasePath );
        String[] output = ssh.getStdout().split( "\\s+" );
        Assert.assertEquals( output[ 2 ], "drwxr-xr-x.", "目录权限不正确" );
        Assert.assertEquals( output[ 4 ] + " " + output[ 5 ],
                "sdbadmin sdbadmin_group", "目录用户不正确" );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        ssh.disconnect();
        sdbSsh.disconnect();
    }
}
