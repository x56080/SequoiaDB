package com.sequoiadb.commlib;

import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterSuite;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.Parameters;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;

public class SdbTestBase {
    public static String coordUrl;
    public static String hostName;
    public static String serviceName;
    public static String csName;
    public static int reservedPortBegin;
    public static int reservedPortEnd;
    public static String reservedDir;
    public static String workDir;
    public static String rootPwd;
    public static String remoteUser;
    public static String remotePwd;
    public static String scriptDir;

    @Parameters({ "HOSTNAME", "SVCNAME", "CHANGEDPREFIX", "RSRVPORTBEGIN", "RSRVPORTEND",
            "RSRVNODEDIR", "WORKDIR", "ROOTPASSWD", "REMOTEUSER", "REMOTEPASSWD", "SCRIPTDIR" })
    @BeforeSuite
    public static void initSuite(String HOSTNAME, String SVCNAME, String COMMCSNAME,
            int RSRVPORTBEGIN, int RSRVPORTEND, String RSRVNODEDIR, String WORKDIR,
            String ROOTPASSWD, String REMOTEUSER, String REMOTEPASSWD, String SCRIPTDIR) {
        hostName = HOSTNAME;
        serviceName = SVCNAME;
        csName = COMMCSNAME;
        reservedPortBegin = RSRVPORTBEGIN;
        reservedPortEnd = RSRVPORTEND;
        reservedDir = RSRVNODEDIR;
        workDir = WORKDIR;
        coordUrl = HOSTNAME + ":" + SVCNAME;
        rootPwd = ROOTPASSWD;
        remoteUser = REMOTEUSER;
        remotePwd = REMOTEPASSWD;
        scriptDir = SCRIPTDIR;
        Sequoiadb db = null;
        try {
            db = new Sequoiadb(coordUrl, "", "");
            boolean ret = createCommonCS(db);
            Assert.assertTrue(ret);
            createWorkDir();
            createReserveDir();
        }
        catch (BaseException e) {
            Assert.fail("connect " + coordUrl + ": " + e.getErrorCode());
        }
        finally {
            if (db != null) {
                db.close();
            }
        }
    }

    private static void createReserveDir() {
        try {
            GroupMgr mgr = new GroupMgr();
            List<String> hosts = mgr.getAllHosts();
            for (String host : hosts) {
                Ssh ssh = new Ssh(host, "root", SdbTestBase.rootPwd);
                try {
                    ssh.exec("mkdir -p " + SdbTestBase.reservedDir);
                    ssh.exec("chown " + SdbTestBase.remoteUser + " " + SdbTestBase.reservedDir);
                }
                finally {
                    ssh.disconnect();
                }
            }
        }
        catch (ReliabilityException e) {
            Assert.fail(e.getMessage());
            e.printStackTrace();
        }
    }

    private static void createWorkDir() {
        try {
            GroupMgr mgr = new GroupMgr();
            List<String> hosts = mgr.getAllHosts();
            for (String host : hosts) {
                Ssh ssh = new Ssh(host, "root", SdbTestBase.rootPwd);
                try {
                    ssh.exec("mkdir -p " + SdbTestBase.workDir);
                }
                finally {
                    ssh.disconnect();
                }
            }
        }
        catch (ReliabilityException e) {
            Assert.fail(e.getMessage());
            e.printStackTrace();
        }
    }

    @AfterSuite(enabled = false)
    public static void finiSuite() {
        System.out.println("finisuit");
        Sequoiadb db = null;
        try {
            db = new Sequoiadb(coordUrl, "", "");
            if (db.isCollectionSpaceExist(csName)) {
                db.dropCollectionSpace(csName);
            }
        }
        catch (BaseException e) {
            e.printStackTrace();
        }
        finally {
            if (db != null) {
                db.close();
            }
        }
    }

    private static boolean createCommonCS(Sequoiadb sdb) {
        boolean isCreateSuccess = true;
        try {
            if (sdb.isCollectionSpaceExist(csName)) {
                sdb.dropCollectionSpace(csName);
            }
            sdb.createCollectionSpace(csName);
        }
        catch (BaseException e) {
            System.out.printf("create CollectionSpace %s failed, errMsg:%s\n", csName,
                    e.getMessage());
            isCreateSuccess = false;
        }
        return isCreateSuccess;
    }

}
