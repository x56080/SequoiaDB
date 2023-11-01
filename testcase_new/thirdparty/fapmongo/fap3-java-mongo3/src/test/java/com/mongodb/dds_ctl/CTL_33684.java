package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33684:同时指定--port和--all启动节点
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33684 extends CTLTestBase {
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

        // 同时指定--all、--port启动节点
        try {
            ssh.exec( ctlPath + " start --all --port " + port1 );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains(
                    "\"--port\" and \"--all\" cannot be specified at the same time" ) ) {
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
