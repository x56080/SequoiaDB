package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33668:指定--configdb参数，值的格式不正确
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/24
 * @UpdateRemark
 * @Version
 */
public class CTL_33668 extends CTLTestBase {
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 初始化路由节点, configdb参数的值格式不正确
        ssh.exec( ctlPath
                + " initrt  --port=16000 --dbpath=/opt/sequoiadds/database/16000 "
                + "--configdb='rs0/localhost:10000.localhost:11000.localhost:12000'" );
        // 启动路由节点, 启动失败
        try {
            ssh.exec( ctlPath + " start --port=16000" );
            Assert.fail( "expected fail but success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "[ERROR] Failed to start node \"16000\"" ) ) {
                throw e;
            }
        }
        // 修正配置文件的配置
        ssh.exec(
                "sed -i 's/rs0\\/localhost:10000.localhost:11000.localhost:12000/"
                        + "rs0\\/localhost:10000,localhost:11000,localhost:12000/g' "
                        + confPath + "16000.yaml" );
        // 启动路由节点, 启动成功
        ssh.exec( ctlPath + " start --all" );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
