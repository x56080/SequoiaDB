package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33652:指定--configsvr部署分片模式（元数据节点），不指定--replname
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/23
 * @UpdateRemark
 * @Version
 */
public class CTL_33652 extends CTLTestBase {
    private Ssh ssh = null;
    private Integer port = 10000;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        try {
            ssh.exec( ctlPath + " initdb --port " + port + " --dbpath "
                    + dataBasePath + port + " --configsvr" );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage()
                    .contains( "must be used with \"--replname\"" ) ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        ssh.disconnect();
    }
}
