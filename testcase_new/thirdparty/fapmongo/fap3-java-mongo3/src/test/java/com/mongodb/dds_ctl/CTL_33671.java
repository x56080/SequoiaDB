package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33671:已经存在27017端口节点，再次创建路由节点不指定--port
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33671 extends CTLTestBase {
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 初始化路由节点, 不指定--port
        ssh.exec( ctlPath + " initrt  --dbpath=/opt/sequoiadds/database/27017 "
                + "--configdb='rs0/localhost:10000,localhost:11000,localhost:12000'" );

        // 检查路由节点端口号
        ssh.exec( ctlPath + " list --all" );
        CommLib.checkNode( ssh.getStdout(), "port", "27017" );

        // 再次初始化路由节点, 不指定--port
        try {
            ssh.exec( ctlPath
                    + " initrt  --dbpath=/opt/sequoiadds/database/30000 "
                    + "--configdb='rs0/localhost:10000,localhost:11000,localhost:12000'" );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            Assert.assertTrue( ssh.getStderr()
                    .contains( "[ERROR] Port \"27017\" already in use" ) );
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
