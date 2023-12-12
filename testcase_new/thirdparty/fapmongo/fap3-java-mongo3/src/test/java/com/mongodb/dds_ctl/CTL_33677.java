package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.dds_ctl.common.basicEntity;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import org.yaml.snakeyaml.Yaml;

/**
 * @Descreption seqDB-33677:在 conf/local 中增加配置文件，创建存放数据文件和日志文件的目录
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/20
 * @UpdateRemark
 * @Version
 */
public class CTL_33677 extends CTLTestBase {
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        Yaml yaml = new Yaml();

        // 创建配置文件
        basicEntity entity = CommLib.createBaseEntity( "file",
                "/opt/sequoiadds/database/10000/mongod.log", true, "0.0.0.0",
                "10000", "/opt/sequoiadds/database/10000", "shard", "shardsvr",
                null );
        ssh.exec( "echo '" + yaml.dumpAsMap( entity ) + "' > " + confPath
                + "10000.yaml" );

        // 将配置文件中不需要的参数去掉
        ssh.exec( "find " + confPath
                + " -type f -exec sed -i '/: null$/d' {} \\;" );

        // 创建数据文件目录
        ssh.exec( "mkdir " + dataBasePath + "10000" );
        // 创建日志文件
        ssh.exec( "touch " + dataBasePath + "10000/mongod.log" );
        // 更改目录、文件所属者
        ssh.exec( "chown -R sdbadmin:sdbadmin_group " + dataBasePath + "10000 "
                + confPath + "10000.yaml" );
        // 启动节点
        ssh.exec( ctlPath + " start --port=10000" );
        // 检查节点状态
        ssh.exec( ctlPath + " list --port=10000" );
        CommLib.checkNode( ssh.getStdout(), "status", "running" );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
