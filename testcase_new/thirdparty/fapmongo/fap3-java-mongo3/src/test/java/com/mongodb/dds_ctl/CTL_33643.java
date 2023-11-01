package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.dds_ctl.common.basicEntity;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption 测试用例 seqDB-33643:configfile文件不指定日志目录
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/20
 * @UpdateRemark
 * @Version
 */
public class CTL_33643 extends CTLTestBase {
    private Ssh ssh = null;
    private String configFileName = "10000.yaml";

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {

        // 指定--configfile部署单个节点，日志目录为空
        basicEntity basicEntity1 = CommLib.createBaseEntity( "file", "", true,
                "0.0.0.0", "10000", "/opt/sequoiadds/database/10000", null,
                null, null );
        try {
            CommLib.createNode( ssh, basicEntity1, configFileName, false );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage()
                    .contains( "Unable to find logpath in file" ) ) {
                throw e;
            }
        }

        // 指定--configfile部署单个节点，日志目录不指定
        basicEntity basicEntity2 = CommLib.createBaseEntity( "file", null, true,
                "0.0.0.0", "10000", "/opt/sequoiadds/database/10000", null,
                null, null );
        try {
            CommLib.createNode( ssh, basicEntity2, configFileName, false );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage()
                    .contains( "Unable to find logpath in file" ) ) {
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
