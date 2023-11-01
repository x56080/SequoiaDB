package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33722:ctl工具指定动作不符合使用方法
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33722 extends CTLTestBase {
    private Ssh ssh = null;
    private Integer port = 10000;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // sdb_dds_ctl工具指定动作，动作不存在
        try {
            ssh.exec( ctlPath + " initdbxxx --port " + port + " --dbpath "
                    + dataBasePath + port );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage()
                    .contains( "unrecognized operation mode \"initdbxxx\"" ) ) {
                throw e;
            }
        }

        // sdb_dds_ctl工具指定动作，动作为非法字符
        try {
            ssh.exec( ctlPath + " @#$% --port " + port + " --dbpath "
                    + dataBasePath + port );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage()
                    .contains( "unrecognized operation mode \"@#$%\"" ) ) {
                throw e;
            }
        }

        // sdb_dds_ctl工具指定动作，同时指定多个
        try {
            ssh.exec( ctlPath + " initdb initrt --port " + port + " --dbpath "
                    + dataBasePath + port );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage()
                    .contains( "too many arguments: initdb initrt" ) ) {
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
