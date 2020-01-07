package com.sequoiadb.metadataconsistency.data;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Random;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.AfterClass;
import org.testng.Assert;
import org.testng.SkipException;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadataconsistency.data.MetaDataUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10161: concurrency[create domain, alter domain, drop domain]
 * 
 * @author xiaoni huang init
 * @Date 2016.9.20
 */

public class Domain10161 extends SdbTestBase {
    private SimpleDateFormat dateFm = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss" );
    private static Sequoiadb sdb = null;
    private static ArrayList< String > dataGroups = null;
    private String domainName = "dm10161";
    private Random random = new Random();
    private int number = 20;
    private int msec = 500;

    @BeforeClass
    public void setUp() {
        // start time
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // judge the mode or group number or node number
            if ( MetaDataUtils.isStandAlone( sdb )
                    || MetaDataUtils.OneGroupMode( sdb )
                    || MetaDataUtils.oneCataNode( sdb ) ) {
                throw new SkipException(
                        "The mode is standlone or only one group or one node, "
                                + "skip the testCase." );
            }
            MetaDataUtils.clearDomain( sdb, domainName );
            dataGroups = MetaDataUtils.getDataGroupNames( sdb );
        } catch ( BaseException e ) {
            sdb.disconnect();
            Assert.fail( e.getMessage() );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            MetaDataUtils.clearDomain( sdb, domainName );
        } catch ( BaseException e ) {
            Assert.fail( "ErrorMsg:\n" + e.getMessage() );
        } finally {
            sdb.disconnect();
        }
    }

    @Test(invocationCount = 3, threadPoolSize = 3)
    public void test() {
        CreateDomain createDomain = new CreateDomain();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        createDomain.start();

        AlterDomain alterDomain = new AlterDomain();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        alterDomain.start();

        DropDomain dropDomain = new DropDomain();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        dropDomain.start();

        if ( !( createDomain.isSuccess() && alterDomain.isSuccess()
                && dropDomain.isSuccess() ) ) {
            Assert.fail( createDomain.getErrorMsg() + alterDomain.getErrorMsg()
                    + dropDomain.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkDomainOfCatalog( domainName );
    }

    private class CreateDomain extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

                BSONObject opt = new BasicBSONObject();
                opt.put( "Groups", dataGroups );
                opt.put( "AutoSplit", false );
                for ( int i = 0; i < 10; i++ ) {
                    db.createDomain(
                            domainName + "_" + random.nextInt( number ), opt );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -215 ) { // -215:Domain already exists
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.disconnect();
            }
        }
    }

    private class AlterDomain extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

                BSONObject opt = new BasicBSONObject();
                int drSize = random.nextInt( dataGroups.size() );
                opt.put( "Groups", dataGroups.get( drSize ).split( "," ) );
                opt.put( "AutoSplit", true );
                for ( int i = 0; i < 10; i++ ) {
                    db.getDomain( domainName + "_" + random.nextInt( number ) )
                            .alterDomain( opt );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -214 ) { // -214:Domain does not exist
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.disconnect();
            }
        }
    }

    private class DropDomain extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

                for ( int i = 0; i < 5; i++ ) {
                    db.dropDomain(
                            domainName + "_" + random.nextInt( number ) );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -214 ) { // -214:Domain does not exist
                    db.disconnect();
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.disconnect();
            }
        }
    }

}
