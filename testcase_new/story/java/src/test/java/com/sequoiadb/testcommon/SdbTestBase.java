package com.sequoiadb.testcommon;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.atomic.AtomicInteger;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterSuite;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Optional;
import org.testng.annotations.Parameters;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextUtils;

public class SdbTestBase {
    protected static String coordUrl;
    protected static String hostName;
    protected static String serviceName;
    protected static String csName;
    protected static int reservedPortBegin;
    protected static int reservedPortEnd;
    protected static String reservedDir;
    protected static String workDir;
    private static String confToolScript;
    private static String enableTransaction;
    private static Sequoiadb sequoiadb = null;
    public static String esHostName;
    public static String esServiceName;
    public static String cappedCSName;
    public static String reservedCL = "java_dummy";

    private static final String TRANSACTIONON = "transactionon";
    private static final String TRANSISOLATION = "transisolation";
    private static final String TRANSLOCKWAIT = "translockwait";
    private static final String INDEXSCANSTEP = "indexscanstep";
    private static final String TRANSTIMEOUT = "transactiontimeout";
    private static final String TRANSAUTOCOMMIT = "transautocommit";
    private static final String TRANSAUTOROLLBACK = "transautorollback";
    private static final String TRANSUSERBS = "transuserbs";
    private static final String NODENAME = "NodeName";
    public static final String RU = "ru";
    public static final String RC = "rc";
    public static final String RCWAITLOCK = "rcwaitlock";
    public static final String RS = "rs";
    public static final String RCAUTO = "rcauto";
    public static final String RCUSERBS = "rcuserbs";

    private static ConfigOptions options = new ConfigOptions();
    public static String testGroup = null;
    private static final int newIndexScanStep = 100;
    public static final int timeOutLen = 120;
    private static final Map< String, BSONObject > group2Conf = new HashMap< String, BSONObject >();
    private static final Map< String, AtomicInteger > group2Count = new HashMap< String, AtomicInteger >();
    private static final Map< String, BSONObject > node2Conf = new HashMap< String, BSONObject >();
    private static boolean istransactionOn = true;
    private static BasicBSONObject confObj = new BasicBSONObject();

    static {
        group2Conf.put( RU, new BasicBSONObject() );
        group2Conf.get( RU ).put( TRANSISOLATION, 0 );
        group2Conf.get( RU ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RU ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RU ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RU ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RU ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RU ).put( TRANSUSERBS, true );

        group2Conf.put( RC, new BasicBSONObject() );
        group2Conf.get( RC ).put( TRANSISOLATION, 1 );
        group2Conf.get( RC ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RC ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RC ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RC ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RC ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RC ).put( TRANSUSERBS, true );

        group2Conf.put( RCWAITLOCK, new BasicBSONObject() );
        group2Conf.get( RCWAITLOCK ).put( TRANSISOLATION, 1 );
        group2Conf.get( RCWAITLOCK ).put( TRANSLOCKWAIT, true );
        group2Conf.get( RCWAITLOCK ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RCWAITLOCK ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RCWAITLOCK ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RCWAITLOCK ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RCWAITLOCK ).put( TRANSUSERBS, true );

        group2Conf.put( RS, new BasicBSONObject() );
        group2Conf.get( RS ).put( TRANSISOLATION, 2 );
        group2Conf.get( RS ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RS ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RS ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RS ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RS ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RS ).put( TRANSUSERBS, true );

        group2Conf.put( RCAUTO, new BasicBSONObject() );
        group2Conf.get( RCAUTO ).put( TRANSISOLATION, 1 );
        group2Conf.get( RCAUTO ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RCAUTO ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RCAUTO ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RCAUTO ).put( TRANSAUTOCOMMIT, true );
        group2Conf.get( RCAUTO ).put( TRANSAUTOROLLBACK, false );
        group2Conf.get( RCAUTO ).put( TRANSUSERBS, true );

        group2Conf.put( RCUSERBS, new BasicBSONObject() );
        group2Conf.get( RCUSERBS ).put( TRANSISOLATION, 1 );
        group2Conf.get( RCUSERBS ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RCUSERBS ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RCUSERBS ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RCUSERBS ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RCUSERBS ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RCUSERBS ).put( TRANSUSERBS, false );

        for ( String key : group2Conf.keySet() ) {
            group2Count.put( key, new AtomicInteger( 0 ) );
            for ( String conf : group2Conf.get( key ).keySet() ) {
                if ( !confObj.containsField( conf ) ) {
                    confObj.put( conf, "" );
                }
            }
        }
    }

    public static synchronized void setRunGroup( List< String > testGroups ) {
        if ( testGroups.size() != 1 ) {
            return;
        }
        if ( !testGroups.get( 0 ).equals( SdbTestBase.testGroup ) ) {
            SdbTestBase.testGroup = testGroups.get( 0 );
        }
    }

    private static void getAllNodeConf( BasicBSONObject selector ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "",
                options )) {
            selector.put( NODENAME, "" );
            selector.put( TRANSACTIONON, "" );

            DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, null,
                    selector, null );
            while ( cursor.hasNext() ) {
                BasicBSONObject doc = ( BasicBSONObject ) cursor.getNext();
                String key = doc.getString( NODENAME );
                doc.remove( NODENAME );
                node2Conf.put( key, doc );

                if ( doc.getString( TRANSACTIONON ).equals( "FALSE" ) ) {
                    istransactionOn = false;
                }
                doc.remove( TRANSACTIONON );
            }
            cursor.close();
        }
    }

    @Parameters({ "HOSTNAME", "SVCNAME", "CHANGEDPREFIX", "RSRVPORTBEGIN",
            "RSRVPORTEND", "RSRVNODEDIR", "WORKDIR", "CONFTOOL",
            "ENABLETRANSACTION", "ESHOSTNAME", "ESSVCNAME", "FULLTEXTPREFIX" })
    @BeforeSuite(alwaysRun = true)
    public static void initSuite( String HOSTNAME, String SVCNAME,
            String COMMCSNAME, int RSRVPORTBEGIN, int RSRVPORTEND,
            String RSRVNODEDIR, String WORKDIR, @Optional("") String CONFTOOL,
            @Optional("false") String ENABLETRANSACTION,
            @Optional("localhost") String ESHOSTNAME,
            @Optional("9200") String ESSVCNAME,
            @Optional("") String FULLTEXTPREFIX ) {
        System.out.println( "initSuite....." );
        hostName = HOSTNAME;
        serviceName = SVCNAME;
        esHostName = ESHOSTNAME;
        esServiceName = ESSVCNAME;
        csName = COMMCSNAME;
        cappedCSName = COMMCSNAME + "_capped";
        reservedPortBegin = RSRVPORTBEGIN;
        reservedPortEnd = RSRVPORTEND;
        reservedDir = RSRVNODEDIR;
        workDir = WORKDIR;
        coordUrl = HOSTNAME + ":" + SVCNAME;
        confToolScript = CONFTOOL;
        enableTransaction = ENABLETRANSACTION;
        FullTextUtils.setFulltextPrefix( FULLTEXTPREFIX );

        try {
            if ( enableTransaction.equals( "true" ) ) {
                getAllNodeConf( confObj );
                if ( !istransactionOn ) {
                    modifyNodeConfAndRestart( true, "before" );
                }
            }

            options.setSocketKeepAlive( true );
            sequoiadb = new Sequoiadb( SdbTestBase.coordUrl, "", "", options );
            if ( sequoiadb.isCollectionSpaceExist( csName ) ) {
                sequoiadb.dropCollectionSpace( csName );
            }
            CollectionSpace cs = sequoiadb.createCollectionSpace( csName );
            cs.createCollection( reservedCL, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{_id:1},AutoSplit:true}" ) );
            // add capped cs;
            if ( sequoiadb.isCollectionSpaceExist( cappedCSName ) ) {
                sequoiadb.dropCollectionSpace( cappedCSName );
            }
            sequoiadb.createCollectionSpace( cappedCSName,
                    ( BSONObject ) JSON.parse( "{Capped:true}" ) );

            File workDirFile = new File( workDir );
            if ( !workDirFile.exists() ) {
                workDirFile.mkdir();
            }

        } catch ( BaseException e ) {
            e.printStackTrace();
            throw new RuntimeException( "initSuite failed" );
        } finally {
            if ( sequoiadb != null ) {
                sequoiadb.close();
            }
        }
    }

    private static BSONObject buildNodeConf( boolean transactionon ) {
        BasicBSONObject configs = new BasicBSONObject();
        configs.append( TRANSACTIONON, transactionon );
        return configs;
    }

    @SuppressWarnings("unused")
    private static BSONObject buildNodeConf( Properties prop ) {
        BasicBSONObject configs = new BasicBSONObject();
        for ( Object key : prop.keySet() ) {
            String value = prop.getProperty( ( String ) key );
            int val;
            try {
                val = Integer.parseInt( value );
                configs.append( ( String ) key, val );
            } catch ( NumberFormatException e ) {
                configs.append( ( String ) key, Boolean.parseBoolean( value ) );
            }
        }
        return configs;
    }

    private static void modifyNodeConfAndRestart( boolean transactionon,
            String mode ) {
        BSONObject defaultConf = new BasicBSONObject();
        BSONObject dataConf = buildNodeConf( transactionon );
        BSONObject stdalnConf = buildNodeConf( transactionon );
        try {
            createConfFile( defaultConf, defaultConf, dataConf, defaultConf,
                    dataConf, defaultConf, stdalnConf, defaultConf );
        } catch ( IOException e1 ) {
            e1.printStackTrace();
            throw new RuntimeException( "initGroups failed!!!" );
        }

        String[] cmd;
        try {
            cmd = getConfCmd( mode, confToolScript );
            if ( !execCmd( cmd ) ) {
                throw new RuntimeException(
                        "exec script failed, initGroups failed!!!" );
            }
        } catch ( IOException | InterruptedException e ) {
            e.printStackTrace();
            throw new RuntimeException( "initGroups failed!!!" );
        }
    }

    private static void modifyNodeConf( BSONObject cfg, BSONObject object ) {
        if ( object == null ) {
            object = new BasicBSONObject().append( "Global", true );
        }
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "",
                options )) {
            sdb.updateConfig( cfg, object );
        } catch ( BaseException e ) {
            e.printStackTrace();
            throw e;
        }
    }

    @BeforeTest(groups = { RU, RC, RCWAITLOCK, RS, RCAUTO, RCUSERBS })
    public static synchronized void initTestGroups() {
        if ( testGroup == null )
            return;
        System.out.println( "init " + testGroup + " Groups..........." );
        modifyNodeConf( group2Conf.get( testGroup ), null );
    }

    @AfterTest(groups = { RC, RU, RCWAITLOCK, RS, RCAUTO,
            RCUSERBS }, alwaysRun = true)
    public static synchronized void finiTestGroups() {
        if ( testGroup == null )
            return;
        System.out.println( "fini " + testGroup + " Groups..........." );
        for ( String key : node2Conf.keySet() ) {
            BasicBSONObject opt = new BasicBSONObject();
            opt.put( NODENAME, key );
            modifyNodeConf( node2Conf.get( key ), opt );
        }
        testGroup = null;
    }

    @AfterSuite(alwaysRun = true)
    public static void finiSuite() {
        try {
            if ( enableTransaction.equals( "true" ) && !istransactionOn ) {
                modifyNodeConfAndRestart( false, "after" );
            }
            sequoiadb = new Sequoiadb( SdbTestBase.coordUrl, "", "", options );

            // if (sequoiadb.isCollectionSpaceExist(csName)) {
            // sequoiadb.dropCollectionSpace(csName);
            // }

            // drop capped cs
            // if (sequoiadb.isCollectionSpaceExist(cappedCSName)) {
            // sequoiadb.dropCollectionSpace(cappedCSName);
            // }
            // sdb.close() ;

            // 检查事务快照
            DBCursor cursor = sequoiadb
                    .getSnapshot( Sequoiadb.SDB_SNAP_TRANSACTIONS, "", "", "" );
            ArrayList< BSONObject > List = new ArrayList<>();
            while ( cursor.hasNext() ) {
                List.add( cursor.getNext() );
            }
            if ( !List.isEmpty() ) {
                System.out.println( "SDB_SNAP_TRANSACTIONS in List:" + List );
                Assert.fail( "SDB_SNAP_TRANSACTIONS is not empty！" );
            }

        } catch ( BaseException e ) {
            e.printStackTrace();
        } finally {
            if ( sequoiadb != null ) {
                sequoiadb.close();
            }

            SdbThreadBase.shutdown();
        }
    }

    public static String getDefaultCoordUrl() {
        return coordUrl;
    }

    public static String getWorkDir() {
        return workDir;
    }

    private static boolean execCmd( String[] cmd )
            throws IOException, InterruptedException {
        System.out.println( "cmd:" + Arrays.toString( cmd ) );
        Process process = Runtime.getRuntime().exec( cmd );

        BufferedReader input = new BufferedReader(
                new InputStreamReader( process.getInputStream() ) );
        String line = "";
        while ( ( line = input.readLine() ) != null ) {
            System.out.println( line );
        }

        int exitValue = process.waitFor();
        if ( 0 != exitValue ) {
            System.out.println(
                    "fail to change node configure beforetest, return code="
                            + exitValue );
            return false;
        } else {
            return true;
        }
    }

    private static String[] getConfCmd( String mode, String confToolScript )
            throws IOException {
        String[] cmd = new String[ 5 ];
        String confFullName = "";
        if ( mode.equals( "before" ) ) {
            confFullName = getCurrentClass().getResource( "" ) + "/node.conf";
            confFullName = confFullName.substring( 5 );
        } else {
            confFullName = System.getProperty( "user.dir" ) + "/node.conf.ini";
        }

        Properties prop = new Properties();
        InputStream in = new FileInputStream(
                new File( "/etc/default/sequoiadb" ) );
        prop.load( in );
        String installPath = prop.getProperty( "INSTALL_DIR" );
        String sdbFullName = installPath + "/bin/sdb";
        System.out.println( sdbFullName );
        cmd[ 0 ] = sdbFullName;
        cmd[ 1 ] = "-f";
        cmd[ 2 ] = confFullName + "," + confToolScript;
        cmd[ 3 ] = "-e";
        cmd[ 4 ] = "var hostname='" + hostName + "';" + "var svcname="
                + serviceName + ";" + "var mode='" + mode + "'";

        return cmd;
    }

    private static void createConfFile( BSONObject cataConf,
            BSONObject cataDynaConf, BSONObject coordConf,
            BSONObject coordDynaConf, BSONObject dataConf,
            BSONObject dataDynaConf, BSONObject stdalnConf,
            BSONObject stdalnDynaConf ) throws IOException {
        String confPath = getCurrentClass().getResource( "" ).getPath()
                + "/node.conf";
        FileWriter confFile = new FileWriter( confPath );
        addStaticConf( confFile, cataConf, coordConf, dataConf, stdalnConf );
        addDynConf( confFile, cataDynaConf, coordDynaConf, dataDynaConf,
                stdalnDynaConf );
        confFile.flush();
        confFile.close();
        System.out.println( "create file: " + confPath );
    }

    @SuppressWarnings("rawtypes")
    private static final Class getCurrentClass() {
        return new Object() {
            public Class getClassForStatic() {
                return this.getClass();
            }
        }.getClassForStatic();
    }

    private static void addStaticConf( FileWriter confFile, BSONObject cataConf,
            BSONObject coordConf, BSONObject dataConf, BSONObject stdalnConf )
            throws IOException {
        confFile.write( "catalogConf = " + cataConf + ";\n" );
        confFile.write( "coordConf = " + coordConf + ";\n" );
        confFile.write( "dataConf = " + dataConf + ";\n" );
        confFile.write( "standaloneConf = " + stdalnConf + ";\n" );
    }

    private static void addDynConf( FileWriter confFile,
            BSONObject cataDynaConf, BSONObject coordDynaConf,
            BSONObject dataDynaConf, BSONObject stdalnDynaConf )
            throws IOException {
        confFile.write( "catalogDynaConf = " + cataDynaConf + ";\n" );
        confFile.write( "coordDynaConf = " + coordDynaConf + ";\n" );
        confFile.write( "dataDynaConf = " + dataDynaConf + ";\n" );
        confFile.write( "standaloneConf = " + stdalnDynaConf + ";\n" );
    }
}
