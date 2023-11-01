package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33659:指定--dbpath的目录无权限
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33659 extends CTLTestBase {
    private Ssh ssh = null;
    private Integer port = 10000;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 提前创建日志、数据目录，但无权限
        ssh.exec( "mkdir -p " + dataBasePath + port );

        // 创建节点
        try {
            ssh.exec( ctlPath + " initdb --port " + port + " --dbpath "
                    + dataBasePath + port );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage()
                    .contains( "There is no wirte permission for" ) ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void teardown() throws Exception {
        ssh.exec( "rm -rf " + dataBasePath + port );
        CommLib.deleteNode( ssh );
        ssh.disconnect();
    }
}
