package com.mongodb.dds_ctl;

import com.mongodb.dds_ctl.common.CTLTestBase;
import com.mongodb.dds_ctl.common.CommLib;
import com.mongodb.utils.Ssh;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Descreption seqDB-33679:使用 root 用户和 sdbadmin 用户 分别创建节点
 * @Author HuangYouquan
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/10/20
 * @UpdateRemark
 * @Version
 */
public class CTL_33679 extends CTLTestBase {
    private Ssh ssh = null;
    private Ssh sshSdbadmin = null;

    @BeforeClass
    public void setup() throws Exception {
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        sshSdbadmin = new Ssh( remoteHost, sdbUser, sdbPwd );
        CommLib.deleteNode( ssh );
    }

    @Test
    public void test() throws Exception {
        // 用root用户创建节点
        createNodes( ssh );
        // 启动节点
        ssh.exec( ctlPath + " start --all" );
        // 检查创建的文件和进程的所属用户
        checkUser( ssh );
        ssh.exec( ctlPath + " stop --all" );
        ssh.exec( ctlPath + " remove --all-nodes" );

        // 用sdbadmin用户创建节点
        createNodes( sshSdbadmin );
        // 启动节点
        ssh.exec( ctlPath + " start --all" );
        // 检查创建的文件和进程的所属用户
        checkUser( sshSdbadmin );
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.deleteNode( ssh );
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }

    private void createNodes( Ssh ssh ) throws Exception {
        ssh.exec( ctlPath + " initdb --replname=rs0 "
                + "--port=10000 --configsvr --dbpath=/opt/sequoiadds/database/10000" );
        ssh.exec( ctlPath + " initdb --replname=shard "
                + "--port=11000 --shardsvr --dbpath=/opt/sequoiadds/database/11000" );
        ssh.exec( ctlPath + " initrt "
                + "--port=12000 --dbpath=/opt/sequoiadds/database/12000 "
                + "--configdb=rs0/localhost:10000" );
    }

    // 检查创建的文件和进程的所属用户
    private void checkUser( Ssh ssh ) throws Exception {
        // 检查文件的所属用户
        ssh.exec( "ls -l " + dataBasePath + "10000" );
        Assert.assertTrue( ssh.getStdout().contains( "sdbadmin" )
                && ssh.getStdout().contains( "sdbadmin_group" ) );
        ssh.exec( "ls -l " + dataBasePath + "11000" );
        Assert.assertTrue( ssh.getStdout().contains( "sdbadmin" )
                && ssh.getStdout().contains( "sdbadmin_group" ) );
        ssh.exec( "ls -l " + dataBasePath + "12000" );
        Assert.assertTrue( ssh.getStdout().contains( "sdbadmin" )
                && ssh.getStdout().contains( "sdbadmin_group" ) );

        // 检查进程的所属用户
        ssh.exec( ctlPath + " list --all" );
        // 获取进程pid
        List< String > list = new ArrayList<>();
        CommLib.getListInfo( ssh.getStdout(), list, CommLib.listType.pid );
        // 检查进程的所属用户
        for ( String pid : list ) {
            ssh.exec( "ps -ef | grep " + pid );
            Assert.assertTrue( ssh.getStdout().contains( "sdbadmin" ) );
        }
    }

}
