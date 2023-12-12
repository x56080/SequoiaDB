package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-33666:initrt 部署路由节点
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/19
 * @UpdateRemark
 * @Version
 */
public class CTL_33666 extends CTLTestBase {

    private Ssh ssh = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 创建多个节点
        createNodes();
        // 启动节点
        ssh.exec( ctlPath + " start --all" );
        // 校验路由节点状态
        ssh.exec( ctlPath + " list --port=" + routePort );
        Assert.assertTrue( ssh.getStdout().contains( "waiting" ) );
        // 初始化config server副本
        ssh.exec( mongoshPath + " --port 10000 --eval=\"rs.initiate()\"" );
        CommLib.checkCluster( 10000 );
        // 再次校验路由节点状态
        ssh.exec( ctlPath + " list --port=" + routePort );
        CommLib.checkNode( ssh.getStdout(), "status", "running" );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }

    private void createNodes() throws Exception {
        ssh.exec( ctlPath
                + " initdb --replname=rs0 --port=10000 --configsvr --dbpath=/opt/sequoiadds/database/10000" );
        ssh.exec( ctlPath
                + " initdb --replname=rs0 --port=11000 --configsvr --dbpath=/opt/sequoiadds/database/11000" );
        ssh.exec( ctlPath
                + " initdb --replname=rs0 --port=12000 --configsvr --dbpath=/opt/sequoiadds/database/12000" );
        ssh.exec( ctlPath
                + " initdb --replname=shard --port=13000 --shardsvr --dbpath=/opt/sequoiadds/database/13000" );
        ssh.exec( ctlPath
                + " initdb --replname=shard --port=14000 --shardsvr --dbpath=/opt/sequoiadds/database/14000" );
        ssh.exec( ctlPath
                + " initdb --replname=shard --port=15000 --shardsvr --dbpath=/opt/sequoiadds/database/15000" );
        ssh.exec( ctlPath
                + " initrt  --port=20000 --dbpath=/opt/sequoiadds/database/20000 "
                + "--configdb=rs0/localhost:10000,localhost:11000,localhost:12000" );
    }
}
