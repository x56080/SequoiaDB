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
 * @Descreption seqDB-33640:configfile文件不指定端口号
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/19
 * @UpdateRemark
 * @Version
 */
public class CTL_33640 extends CTLTestBase {
    private Ssh ssh = null;
    private String configFileName = "10000.yaml";

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {

        // 指定--configfile部署单个节点，端口号为空
        basicEntity basicEntity1 = CommLib.createBaseEntity( "file",
                "/opt/sequoiadds/database/10000/mongod.log", true, "0.0.0.0",
                "", "/opt/sequoiadds/database/10000", null, null, null );
        try {
            CommLib.createNode( ssh, basicEntity1, configFileName, false );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "Unable to find port in file" ) ) {
                throw e;
            }
        }

        // 指定--configfile部署单个节点，端口号不指定
        basicEntity basicEntity2 = CommLib.createBaseEntity( "file",
                "/opt/sequoiadds/database/10000/mongod.log", true, "0.0.0.0",
                null, "/opt/sequoiadds/database/10000", null, null, null );
        try {
            CommLib.createNode( ssh, basicEntity2, configFileName, false );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "Unable to find port in file" ) ) {
                throw e;
            }
        }

        // 指定--configfile部署单个节点，端口号为字符串
        basicEntity basicEntity3 = CommLib.createBaseEntity( "file",
                "/opt/sequoiadds/database/10000/mongod.log", true, "0.0.0.0",
                "abcde", "/opt/sequoiadds/database/10000", null, null, null );
        try {
            CommLib.createNode( ssh, basicEntity3, configFileName, false );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "Unable to find port in file" ) ) {
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
