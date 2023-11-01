package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33723:ctl工具指定参数不符合使用方法
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33723 extends CTLTestBase {
    private Ssh ssh = null;
    private Integer port = 10000;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // sdb_dds_ctl工具指定参数，参数不存在
        try {
            ssh.exec( ctlPath + " initdb --port " + port + " --dbpath "
                    + dataBasePath + port + " --abcd efg" );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "无法识别的选项“--abcd”" ) ) {
                throw e;
            }
        }

        // sdb_dds_ctl工具指定参数，参数为非法字符
        try {
            ssh.exec( ctlPath + " initdb --port " + port + " --dbpath "
                    + dataBasePath + port + " --@#$% efg" );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "sdb_dds_ctl：无法识别的选项“--@#$%”" ) ) {
                throw e;
            }
        }

        // sdb_dds_ctl工具指定参数，相同参数同时指定多次
        try {
            ssh.exec( ctlPath + " initdb initrt --port " + port + " --dbpath "
                    + dataBasePath + port + " --port " + port );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage()
                    .contains( "The parameter --port cannot be reused" ) ) {
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
