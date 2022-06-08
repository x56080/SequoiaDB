package com.sequoiadb.stp;

import org.testng.Assert;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.Ssh;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;

public class StpUtils extends SdbTestBase {

    private String user;
    private String passwd;
    private Ssh ssh;
    private int port;
    private String remotePath;
    private final String localScriptPath = SdbTestBase.scriptDir;
    private final String scriptName = "StpUtil.js";

    public StpUtils() {
        this.user = "root";
        this.passwd = SdbTestBase.rootPwd;
        this.port = 22;
        this.remotePath = SdbTestBase.workDir;
    }

    // 获取stp 节点
    public String getStpInfo( String hostName, String svcName, String type )
            throws BaseException, ReliabilityException {
        String info = "";
        try {
            ssh = new Ssh( hostName, user, passwd, port );
            ssh.scpTo( localScriptPath + "/" + scriptName, remotePath + "/" );
            String sdbInstallDir = ssh.getSdbInstallDir();
            String cmd = sdbInstallDir + "/bin/sdb -f " + remotePath + "/"
                    + scriptName + " -e 'var STPHOSTNAME=\"" + hostName
                    + "\";var STPSVCNAME=" + svcName + "; var FUNCTION=\""
                    + type + "\"'";
            ssh.exec( cmd );
            info = ssh.getStdout().substring( 0, ssh.getStdout().length() - 1 );
        } catch ( ReliabilityException e ) {
            throw e;
        }
        return info;
    }

    // 停止stp节点
    public String stopStpNode( String hostName )
            throws BaseException, ReliabilityException {
        String rc = "true";
        try {
            ssh = new Ssh( hostName, user, passwd, port );
            String sdbInstallDir = ssh.getSdbInstallDir();
            ssh.exec( sdbInstallDir + "/bin/stpstop" );
            rc = ssh.getStdout().substring( 0, ssh.getStdout().length() - 1 );
        } catch ( ReliabilityException e ) {
            throw e;
        }
        return rc;
    }

    // 启动stp节点
    public String startStpNode( String hostName )
            throws BaseException, ReliabilityException {
        String rc = "true";
        try {
            ssh = new Ssh( hostName, user, passwd, port );
            String sdbInstallDir = ssh.getSdbInstallDir();
            ssh.exec( sdbInstallDir + "/bin/stpstart" );
            rc = ssh.getStdout().substring( 0, ssh.getStdout().length() - 1 );
        } catch ( ReliabilityException e ) {
            throw e;
        }
        return rc;
    }

    public void checkTime( String primaryTime1, String spareTime1,
            String primaryTime2 ) {
        long time1 = Long.parseLong( primaryTime1.split( "," )[ 0 ] );
        long timeerror1 = Long.parseLong( primaryTime1.split( "," )[ 1 ] );

        long spareTime = Long.parseLong( spareTime1.split( "," )[ 0 ] );
        long spareTimeerror = Long.parseLong( spareTime1.split( "," )[ 0 ] );

        long time2 = Long.parseLong( primaryTime2.split( "," )[ 0 ] );
        long timeerror2 = Long.parseLong( primaryTime2.split( "," )[ 1 ] );

        long maxtimeerror1 = Math.max( timeerror1, spareTimeerror );
        long maxtimeerror2 = Math.max( timeerror2, spareTimeerror );
        if ( spareTime < time1 - maxtimeerror1
                || spareTime > time2 + maxtimeerror2 ) {
            Assert.fail( "time is not expected, primaryTime1 : " + primaryTime1
                    + " spareTime1 : " + spareTime1 + " primaryTime2 : "
                    + primaryTime2 );
        }

    }
}