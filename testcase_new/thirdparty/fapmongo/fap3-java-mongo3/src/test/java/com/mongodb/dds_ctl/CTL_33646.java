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
 * @Descreption seqDB-33646:configfile文件中参数对应的数据目录或日志目录无权限
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/20
 * @UpdateRemark
 * @Version
 */
public class CTL_33646 extends CTLTestBase {
    private Ssh rootSsh = null;
    private Ssh sdbSsh = null;
    private String configFileName = "10000.yaml";

    @BeforeClass
    public void setup() throws Exception {
        rootSsh = new Ssh( remoteHost, remoteUser, remotePwd );
        sdbSsh = new Ssh( remoteHost, sdbUser, sdbPwd );
        CommLib.deleteNode( rootSsh );
    }

    @Test
    public void test() throws Exception {
        // 提前用root用户创建数据目录和日志目录
        rootSsh.exec( "mkdir -p " + dataBasePath + "10000" );

        basicEntity basicEntity = CommLib.createBaseEntity( "file",
                "/opt/sequoiadds/database/10000/mongod.log", true, "0.0.0.0",
                "10000", "/opt/sequoiadds/database/10000", null, null, null );
        try {
            CommLib.createNode( sdbSsh, basicEntity, configFileName, false );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "There is no wirte permission" ) ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void teardown() throws Exception {
        rootSsh.exec( "rm -rf " + configFilePath + configFileName );
        rootSsh.exec( "rm -rf " + dataBasePath + "10000" );
        CommLib.deleteNode( rootSsh );
        rootSsh.disconnect();
        sdbSsh.disconnect();
    }
}
