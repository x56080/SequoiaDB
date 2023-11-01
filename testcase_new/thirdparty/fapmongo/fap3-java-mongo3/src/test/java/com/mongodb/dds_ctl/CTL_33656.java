package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33656:指定--dbpath的目录不存在
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/23
 * @UpdateRemark
 * @Version
 */
public class CTL_33656 extends CTLTestBase {
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
    }
}
