package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.dds_ctl.common.basicEntity;
import com.mongodb.dds_ctl.common.extraEntity;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import org.yaml.snakeyaml.Yaml;

/**
 * @Descreption seqDB-33645:configfile文件中的额外参数书写错误
 * @Author wangxingming
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/20
 * @UpdateRemark
 * @Version
 */
public class CTL_33645 extends CTLTestBase {
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

        // 指定--configfile部署单个节点，额外参数书写错误
        basicEntity basicEntity = CommLib.createBaseEntity( "file",
                "/opt/sequoiadds/database/10000/mongod.log", true, "0.0.0.0",
                "10000", "/opt/sequoiadds/database/10000", null, null, null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity ) + "' > "
                + configFilePath + configFileName );

        // 配置文件追加额外参数
        extraEntity extraEntity = CommLib.createExtraEntity( true, null );
        ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity ) + "' >> "
                + configFilePath + configFileName );

        // 将配置文件中fork修改为fork123
        ssh.exec( "sed -i 's/fork:/fork123:/g' " + configFilePath
                + configFileName );

        // 将配置文件中不需要的参数去掉
        ssh.exec( "find " + configFilePath
                + " -type f -exec sed -i '/: null$/d' {} \\;" );

        // 节点部署成功
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + configFileName );

        try {
            // 节点启动失败
            ssh.exec( ctlPath + " start --all" );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "Failed to start node" ) ) {
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
