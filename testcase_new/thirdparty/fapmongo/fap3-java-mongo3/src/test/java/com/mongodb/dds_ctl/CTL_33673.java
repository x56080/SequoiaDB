package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33673:创建路由节点不指定--bindip
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33673 extends CTLTestBase {
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 初始化路由节点, 不指定--bindip
        ssh.exec( ctlPath
                + " initrt  --port=16000 --dbpath=/opt/sequoiadds/database/16000 "
                + "--configdb='rs0/localhost:10000,localhost:11000,localhost:12000'" );

        // 检查路由节点绑定IP是否为 0.0.0.0
        ssh.exec( "grep 'bindIp:' " + confPath
                + "16000.yaml | awk '{print $2}'" );
        Assert.assertTrue( ssh.getStdout().contains( "0.0.0.0" ) );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
