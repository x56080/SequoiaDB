package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33675:修改 conf/local 中配置文件的端口号，一并更改文件名为对应的端口号 <PORT>.yaml
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/19
 * @UpdateRemark
 * @Version
 */
public class CTL_33675 extends CTLTestBase {
    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 初始化单个节点
        ssh.exec( ctlPath
                + " initdb --replname=rs0 --port=10000 --configsvr --dbpath=/opt/sequoiadds/database/10000" );
        // 启动节点
        ssh.exec( ctlPath + " start --port=10000" );
        // 停止节点
        ssh.exec( ctlPath + " stop --port=10000" );
        // 修改节点配置文件的端口号
        ssh.exec( "sed -i 's/port: 10000/port: 12000/' " + confPath
                + "10000.yaml" );
        // 修改节点配置文件名
        ssh.exec( "mv " + confPath + "10000.yaml " + confPath + "12000.yaml" );
        // 重启节点
        ssh.exec( ctlPath + " start --port=12000" );
        // 检查节点的端口
        ssh.exec( ctlPath + " list --port=12000" );
        CommLib.checkNode( ssh.getStdout(), "port", "12000" );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
