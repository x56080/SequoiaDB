package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.dds_ctl.common.basicEntity;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import org.yaml.snakeyaml.Yaml;

/**
 * @Descreption seqDB-33644:configfile文件中的必要参数书写错误
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/20
 * @UpdateRemark
 * @Version
 */
public class CTL_33644 extends CTLTestBase {
    private Ssh ssh = null;
    private String configFileName = "10000.yaml";

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        Yaml yaml = new Yaml();

        // 指定--configfile部署单个节点，端口参数书写错误
        basicEntity basicEntity1 = CommLib.createBaseEntity( "file",
                "/opt/sequoiadds/database/10000/mongod.log", true, "0.0.0.0",
                "10000", "/opt/sequoiadds/database/10000", null, null, null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity1 ) + "' > "
                + configFilePath + configFileName );
        // 将配置文件中port修改为port123
        ssh.exec( "sed -i 's/port:/port123:/g' " + configFilePath
                + configFileName );
        // 将配置文件中不需要的参数去掉
        ssh.exec( "find " + configFilePath
                + " -type f -exec sed -i '/: null$/d' {} \\;" );
        try {
            ssh.exec( ctlPath + " init --configfile " + configFilePath
                    + configFileName );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "Unable to find port in file" ) ) {
                throw e;
            }
        }

        // 指定--configfile部署单个节点，日志目录参数书写错误
        basicEntity basicEntity2 = CommLib.createBaseEntity( "file",
                "/opt/sequoiadds/database/10000/mongod.log", true, "0.0.0.0",
                "10000", "/opt/sequoiadds/database/10000", null, null, null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity2 ) + "' > "
                + configFilePath + configFileName );
        // 将配置文件中port修改为port123
        ssh.exec( "sed -i 's/path:/path123:/g' " + configFilePath
                + configFileName );
        // 将配置文件中不需要的参数去掉
        ssh.exec( "find " + configFilePath
                + " -type f -exec sed -i '/: null$/d' {} \\;" );
        try {
            ssh.exec( ctlPath + " init --configfile " + configFilePath
                    + configFileName );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage()
                    .contains( "Unable to find logpath in file" ) ) {
                throw e;
            }
        }

        // 指定--configfile部署单个节点，数据目录参数书写错误
        basicEntity basicEntity3 = CommLib.createBaseEntity( "file",
                "/opt/sequoiadds/database/10000/mongod.log", true, "0.0.0.0",
                "10000", "/opt/sequoiadds/database/10000", null, null, null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity3 ) + "' > "
                + configFilePath + configFileName );
        // 将配置文件中port修改为port123
        ssh.exec( "sed -i 's/dbPath:/dbPath123:/g' " + configFilePath
                + configFileName );
        // 将配置文件中不需要的参数去掉
        ssh.exec( "find " + configFilePath
                + " -type f -exec sed -i '/: null$/d' {} \\;" );
        try {
            ssh.exec( ctlPath + " init --configfile " + configFilePath
                    + configFileName );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "Unable to find dbpath in file" ) ) {
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
