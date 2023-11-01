package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33658:指定--dbpath的目录不为空
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33658 extends CTLTestBase {
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
        // 提前创建日志、数据目录；并在目录下创建文件
        sdbSsh.exec( "mkdir -p " + dataBasePath + port );
        sdbSsh.exec( "touch " + dataBasePath + port + "/mongodb.log" );

        // 创建节点
        try {
            ssh.exec( ctlPath + " initdb --port " + port + " --dbpath "
                    + dataBasePath + port );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "is not empty" ) ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void teardown() throws Exception {
        ssh.exec( "rm -rf " + dataBasePath + port );
        CommLib.deleteNode( ssh );
        ssh.disconnect();
        sdbSsh.disconnect();
    }
}
