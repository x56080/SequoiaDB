package com.sequoias3.testcommon;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import org.testng.Assert;
import org.testng.annotations.AfterSuite;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Optional;
import org.testng.annotations.Parameters;

import java.io.File;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class S3TestBase {
    public static final String PARTLISTINUSEOFF = "partlistinuseoff";
    public static final String PARTSIZELIMITOFF = "partsizelimitoff";
    public static final String CONTEXTLIFECYCLECONF = "contextlifecycleconf";
    public static final String AUTHORIZATIONOFF = "authorizationoff";
    public static final String ALLOWREPUTON = "allowreputon";
    private static final String PARTLISTINUSE = "sdbs3.multipartupload.partlistinuse";
    private static final String PARTSIZELIMIT = "sdbs3.multipartupload.partsizelimit";
    private static final String CONTEXTLIFECYCLE = "sdbs3.context.lifecycle";
    private static final String AUTHORIZATION = "sdbs3.authorization.check";
    private static final String ALLOWREPUT = "sdbs3.bucket.allowreput";
    private static final Map<String, Map<String, String>> group2Conf = new HashMap<String, Map<String, String>>();
    protected static String coordUrl;
    protected static String hostName;
    protected static String serviceName;
    protected static String s3ClientUrl;
    protected static String s3HostName;
    protected static String s3Port;
    protected static String csName;
    protected static String bucketName;
    protected static String enableVerBucketName;
    protected static int reservedPortBegin;
    protected static int reservedPortEnd;
    protected static String reservedDir;
    protected static String workDir;
    protected static String s3UserName;
    protected static String s3AccessKeyId;
    protected static String remoteuser;
    protected static String remotepasswd;
    protected static String installPath;
    private static String propertiesFileName = "";
    private static String replaceFileName = "";
    private static String clusterFileName = "";
    private static String clusterInfo = "";
    private static String testGroup = null;
    private static StorageInterface storage = new SdbStorage();

    static {
        Map<String, String> partListiNuseOffMap = new HashMap<>();
        partListiNuseOffMap.put( PARTLISTINUSE, "false" );
        group2Conf.put( PARTLISTINUSEOFF, partListiNuseOffMap );

        Map<String, String> partSizeLimitOffMap = new HashMap<>();
        partSizeLimitOffMap.put( PARTSIZELIMIT, "false" );
        group2Conf.put( PARTSIZELIMITOFF, partSizeLimitOffMap );

        Map<String, String> contextLifeCycleConfMap = new HashMap<>();
        contextLifeCycleConfMap.put( CONTEXTLIFECYCLE, "2" );
        group2Conf.put( CONTEXTLIFECYCLECONF, contextLifeCycleConfMap );

        Map<String, String> authorizationMap = new HashMap<>();
        authorizationMap.put( AUTHORIZATION, "false" );
        group2Conf.put( AUTHORIZATIONOFF, authorizationMap );

        Map<String, String> allowReputMap = new HashMap<>();
        allowReputMap.put( ALLOWREPUT, "true" );
        group2Conf.put( ALLOWREPUTON, allowReputMap );
    }

    public static synchronized void setRunGroup( List<String> testGroups ) {
        if ( testGroups.size() != 1 ) {
            return;
        }
        if ( !testGroups.get( 0 ).equals( S3TestBase.testGroup ) ) {
            S3TestBase.testGroup = testGroups.get( 0 );
        }
    }

    @Parameters({ "HOSTNAME", "SVCNAME", "CHANGEDPREFIX", "RSRVPORTBEGIN",
            "RSRVPORTEND", "RSRVNODEDIR", "WORKDIR", "S3HOSTNAME", "S3PORT",
            "S3USERNAME", "S3ACCESSKEYID", "REMOTEUSER", "REMOTEPASSWD" })
    @BeforeSuite(alwaysRun = true)
    public static void initSuite( String HOSTNAME, String SVCNAME,
            String COMMCSNAME, int RSRVPORTBEGIN, int RSRVPORTEND,
            String RSRVNODEDIR, String WORKDIR, String S3HOSTNAME,
            @Optional("8002") String S3PORT, String S3USERNAME,
            String S3ACCESSKEYID, String REMOTEUSER, String REMOTEPASSWD )
            throws Exception {
        hostName = HOSTNAME;
        serviceName = SVCNAME;
        csName = COMMCSNAME;
        reservedPortBegin = RSRVPORTBEGIN;
        reservedPortEnd = RSRVPORTEND;
        reservedDir = RSRVNODEDIR;
        workDir = WORKDIR;
        coordUrl = HOSTNAME + ":" + SVCNAME;
        s3HostName = S3HOSTNAME;
        s3Port = S3PORT;
        s3UserName = S3USERNAME;
        s3AccessKeyId = S3ACCESSKEYID;
        s3ClientUrl = "http://" + S3HOSTNAME + ":" + S3PORT;
        bucketName = "commbucket";
        enableVerBucketName = "commbucketwithversion";
        remoteuser = REMOTEUSER;
        remotepasswd = REMOTEPASSWD;

        getInstallPath();

        storage.envPrePare( coordUrl );
        changeConfAndStartS3();
        // clean file
        File workDirFile = new File( workDir );
        if ( !workDirFile.exists() ) {
            workDirFile.mkdir();
        }

        AmazonS3 s3Client = null;
        try {
            // clean up existing buckets
            s3Client = CommLib.buildS3Client();
            List<Bucket> buckets = s3Client.listBuckets();
            for ( int i = 0; i < buckets.size(); i++ ) {
                String bucketName = buckets.get( i ).getName();
                String bucketVerStatus = s3Client
                        .getBucketVersioningConfiguration( bucketName )
                        .getStatus();
                if ( bucketVerStatus == "null" ) {
                    CommLib.deleteAllObjects( s3Client, bucketName );
                } else {
                    CommLib.deleteAllObjectVersions( s3Client, bucketName );
                }
                s3Client.deleteBucket( bucketName );
            }

            // create bucket
            s3Client.createBucket( new CreateBucketRequest( bucketName ) );

            // create bucket by enable versioning
            s3Client.createBucket(
                    new CreateBucketRequest( enableVerBucketName ) );
            CommLib.setBucketVersioning( s3Client, enableVerBucketName,
                    "Enabled" );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }

    }

    @BeforeTest(groups = { PARTLISTINUSEOFF, PARTLISTINUSEOFF,
            CONTEXTLIFECYCLECONF, AUTHORIZATIONOFF,
            ALLOWREPUTON }, alwaysRun = true)
    public static synchronized void initTestGroups() throws Exception {
        if ( testGroup == null )
            return;
        System.out.println( "init " + testGroup + " Groups..........." );
        execCmd( Command.S3_CHANGECONF_BEFORETEST );
        execCmd( Command.S3_STOP );
        execCmd( Command.S3_START );
    }

    @AfterTest(groups = { PARTLISTINUSEOFF, PARTLISTINUSEOFF,
            CONTEXTLIFECYCLECONF, AUTHORIZATIONOFF,
            ALLOWREPUTON }, alwaysRun = true)
    public static synchronized void finiTestGroups() throws Exception {
        if ( testGroup == null )
            return;
        System.out.println( "fini " + testGroup + " Groups..........." );
        execCmd( Command.S3_CHANGECONF_AFTERTEST );
        testGroup = null;
        execCmd( Command.S3_STOP );
        execCmd( Command.S3_START );
    }

    @AfterSuite(alwaysRun = true)
    public static void finiSuite() throws Exception {
        try {
            execCmd( Command.S3_RESTORECONF );
            clusterFileName = installPath + "/tools/sequoias3/log/cluster.log";
            clusterInfo = storage.getClusterInfo( coordUrl );
            execCmd( Command.S3_SAVECLUSTERINFO );
            storage.envRestore( coordUrl );
        } finally {
            execCmd( Command.S3_STOP );
        }
    }

    public static String getDefaultCoordUrl() {
        return coordUrl;
    }

    public static String getWorkDir() {
        return workDir;
    }

    public static String getDefaultS3ClientUrl() {
        return s3ClientUrl;
    }

    public static void changeConfAndStartS3() throws Exception {
        // 更新properties
        try {
            execCmd( Command.S3_SETCONFBEFORE );
            // change log level
            execCmd( Command.S3_CHANGEDIALEVEL );
        } catch ( Exception e ) {
            e.printStackTrace();
            Assert.fail( "update application.properties file failed" );
        }
        System.out.println( "finish update application.properties" );
        execCmd( Command.S3_CHECKPORTALIVE );
        String output = Command.S3_CHECKPORTALIVE.getOutput();
        //检查如已存在s3进程，则重启s3服务，不存在的话就直接启动s3
        if ( output.contains( "sequoias3" ) ) {
            System.out.println( "restart s3..." );
            execCmd( Command.S3_STOP );
            execCmd( Command.S3_START );
        } else {
            execCmd( Command.S3_START );
        }
    }

    public static void getInstallPath() throws Exception {
        Command.S3_GETINSTALLPATH.execCmd( s3HostName, remoteuser, remotepasswd,
                Command.S3_GETINSTALLPATH.cmd );
        installPath = Command.S3_GETINSTALLPATH.getOutput();
    }

    private static void execCmd( Command cmd ) throws Exception {
        String command = "";
        switch ( cmd ) {
        case S3_CHECKPORTALIVE:
        case S3_START:
        case S3_STOP:
            cmd.exec( s3HostName, remoteuser, remotepasswd, installPath );
            break;
        case S3_SETCONFBEFORE:
            propertiesFileName = installPath
                    + "/tools/sequoias3/config/application.properties";
            replaceFileName = installPath
                    + "/tools/sequoias3/config/ori_application.properties";
            String coordUrls = storage.getUrls( coordUrl );
            cmd.exec( s3HostName, remoteuser, remotepasswd, propertiesFileName,
                    replaceFileName, s3Port, coordUrls, propertiesFileName );
            break;
        case S3_CHANGEDIALEVEL:
            String logBackFileName =
                    installPath + "/tools/sequoias3/config/logback.xml";
            cmd.exec( s3HostName, remoteuser, remotepasswd, logBackFileName );
            break;
        case S3_RESTORECONF:
            cmd.exec( s3HostName, remoteuser, remotepasswd, propertiesFileName,
                    replaceFileName, propertiesFileName );
            break;
        case S3_CHANGECONF_BEFORETEST:
            String config = group2Conf.get( testGroup ).toString()
                    .replace( "{", "" ).replace( "}", "" );
            cmd.exec( s3HostName, remoteuser, remotepasswd, config,
                    propertiesFileName );
            break;
        case S3_CHANGECONF_AFTERTEST:
            String conf = group2Conf.get( testGroup ).toString()
                    .replace( "{", "" ).replace( "}", "" );
            cmd.exec( s3HostName, remoteuser, remotepasswd, conf, conf,
                    propertiesFileName );
            break;
        case S3_SAVECLUSTERINFO:
            cmd.exec( s3HostName, remoteuser, remotepasswd, clusterInfo,
                    clusterFileName );
            break;
        default:
            break;
        }

        Ssh ssh = null;
        try {
            ssh = new Ssh( s3HostName, remoteuser, remotepasswd );
            ssh.exec( command );
            if ( ssh.getExitStatus() != 0 ) {
                throw new Exception(
                        "exec command : " + command + " failed, stout= " + ssh
                                .getStdout() );
            }
        } finally {
            if ( ssh != null ) {
                ssh.disconnect();
            }
        }
    }

    enum Command {
        S3_CHECKPORTALIVE( "%s/tools/sequoias3/sequoias3.sh status" ), S3_START(
                "source /etc/profile;%s/tools/sequoias3/sequoias3.sh start > /tmp/s3start.log" ), S3_STOP(
                "%s/tools/sequoias3/sequoias3.sh stop -a" ), S3_SETCONFBEFORE(
                "mv %s %s;echo 'server.port=%s\nsdbs3.sequoiadb.url=sequoiadb://%s\nsdbs3.multipartupload.completereservetime=1' > %s" ), S3_CHANGEDIALEVEL(
                "sed -i 's/INFO/DEBUG/g' %s" ), S3_RESTORECONF(
                "rm -f %s;mv %s %s" ), S3_CHANGECONF_BEFORETEST(
                "echo '%s' >> %s" ), S3_CHANGECONF_AFTERTEST(
                "sed -i 's/%s/#%s/g' %s" ), S3_SAVECLUSTERINFO(
                "echo %s > %s" ), S3_GETINSTALLPATH(
                "cat /etc/default/sequoiadb | grep 'INSTALL_DIR' | awk -F '=' '{printf(\"%s\",$2)}'" );

        private String cmd;
        private String output;

        private Command( String cmd ) {
            this.cmd = cmd;
        }

        public void execCmd( String remoteHost, String user, String password,
                String command ) throws Exception {
            Ssh ssh = null;
            try {
                ssh = new Ssh( remoteHost, user, password );
                ssh.exec( command );
                if ( ssh.getExitStatus() != 0 ) {
                    throw new Exception(
                            "exec command : " + command + " failed, stout= "
                                    + ssh.getStdout() );
                }
                this.output = ssh.getStdout();
            } finally {
                if ( ssh != null ) {
                    ssh.disconnect();
                }
            }
        }

        public void exec( String remoteHost, String user, String password,
                String... args ) throws Exception {
            String command = String.format( this.cmd, args );
            execCmd( remoteHost, user, password, command );
        }

        public String getOutput() {
            return this.output;
        }
    }
}
