package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33870 : 不指定--port,--all 启动和停止节点
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/25
 * @UpdateRemark
 * @Version
 */
public class CTL_33870 extends CTLTestBase {
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
                + " initdb --replname=rs0 --port=10000 --configsvr --dbpath=/opt/sequoiadds/database/10000" );
        // 不指定--port,--all 启动节点
        try {
            ssh.exec( ctlPath + " start" );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            Assert.assertTrue( ssh.getStderr().contains(
                    "[ERROR] \"start\" needs to be used with \"--port\" or \"--all\"" ) );
        }
        // 不指定--port,--all 停止节点
        try {
            ssh.exec( ctlPath + " stop" );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            Assert.assertTrue( ssh.getStderr().contains(
                    "[ERROR] \"stop\" needs to be used with \"--port\" or \"--all\"" ) );
        }
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }

}
