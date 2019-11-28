package com.sequoiadb.testcommon;

import java.io.IOException;
import java.io.File;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import org.testng.Assert;
import org.testng.annotations.AfterSuite;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.Parameters;

public class SdbTestBase {
    protected static String coordUrl;
    protected static String hostName;
    protected static String serviceName;
    protected static String csName;
    protected static int reservedPortBegin;
    protected static int reservedPortEnd;
    protected static String reservedDir;
    protected static String workDir;
    protected static String rootPassword;

    @Parameters({ "HOSTNAME", "SVCNAME", "CHANGEDPREFIX", "RSRVPORTBEGIN",
            "RSRVPORTEND", "RSRVNODEDIR", "WORKDIR", "ROOTPASSWD" })
    @BeforeSuite
    public static void initSuite( String HOSTNAME, String SVCNAME,
            String COMMCSNAME, int RSRVPORTBEGIN, int RSRVPORTEND,
            String RSRVNODEDIR, String WORKDIR, String ROOTPASSWD ) {
        hostName = HOSTNAME;
        serviceName = SVCNAME;
        csName = COMMCSNAME;
        reservedPortBegin = RSRVPORTBEGIN;
        reservedPortEnd = RSRVPORTEND;
        reservedDir = RSRVNODEDIR;
        workDir = WORKDIR;
        rootPassword = ROOTPASSWD;
        coordUrl = HOSTNAME + ":" + SVCNAME;

        Sequoiadb db = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            boolean ret = createCommonCS( db );
            Assert.assertTrue( ret );
            File workDirFile = new File( workDir );
            if ( !workDirFile.exists() ) {
                workDirFile.mkdir();
            }
            Runtime runtime = Runtime.getRuntime();
            String command = "chown sdbadmin:sdbadmin_group " + workDirFile;
            Process process = runtime.exec( command );
            int exitValue = process.waitFor();
            if ( exitValue != 0 ) {
                Assert.fail( "failed to exec : " + command );
            }
        } catch ( BaseException | InterruptedException | IOException e ) {
            e.printStackTrace();
            Assert.fail( "failed to initSuite" );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    @AfterSuite
    public static void finiSuite() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
        } catch ( BaseException e ) {

        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    private static boolean createCommonCS( Sequoiadb sdb ) {
        boolean isCreateSuccess = true;
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            sdb.createCollectionSpace( csName );
        } catch ( BaseException e ) {
            System.out.printf( "create CollectionSpace %s failed, errMsg:%s\n",
                    csName, e.getMessage() );
            isCreateSuccess = false;
        }
        return isCreateSuccess;
    }

}
