package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33642:configfile文件中的参数为键值对格式
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/20
 * @UpdateRemark
 * @Version
 */
public class CTL_33642 extends CTLTestBase {
    private Ssh ssh = null;
    private String configFileName = "10000.yaml";

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        ssh.exec( "echo 'logpath = /opt/sequoiadds/database/10000/mongod.log\n"
                + "logappend = true\n"
                + "dbPath = /opt/sequoiadds/database/10000\n" + "fork = true\n"
                + "port = 10000\n" + "bind_ip = 0.0.0.0' > " + configFilePath
                + configFileName );

        try {
            ssh.exec( ctlPath + " init --configfile " + configFilePath
                    + configFileName );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "Unable to find port in file" ) ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void teardown() throws Exception {
        ssh.exec( "rm -rf " + configFilePath + configFileName );
        CommLib.deleteNode( ssh );
        ssh.disconnect();
    }
}
