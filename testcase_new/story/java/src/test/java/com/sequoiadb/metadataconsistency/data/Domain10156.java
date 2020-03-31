package com.sequoiadb.metadataconsistency.data;

import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10156: concurrency[create domain, alter domain]
 * 
 * @author xiaoni huang init
 * @Date 2016.9.20
 */

public class Domain10156 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private static ArrayList< String > dataGroups = null;
    private String domainName = "dm10156";
    private Random random = new Random();
    private int number = 20;
    private int msec = 500;

    @BeforeClass
    public void setUp() {
        // start time
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // judge the mode or group number or node number
            if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb )
                    || MetaDataUtils.oneCataNode( sdb ) ) {
                throw new SkipException(
                        "The mode is standlone or only one group or one node, "
                                + "skip the testCase." );
            }
            MetaDataUtils.clearDomain( sdb, domainName );
            dataGroups = MetaDataUtils.getDataGroupNames( sdb );
        } catch ( BaseException e ) {
            sdb.close();
            Assert.fail( e.getMessage() );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            MetaDataUtils.clearDomain( sdb, domainName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.close();
        }
    }

    @Test(invocationCount = 10, threadPoolSize = 10)
    public void test() {
        CreateDomain createDomain = new CreateDomain();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        createDomain.start();

        AlterDomain alterDomain = new AlterDomain();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        alterDomain.start();

        if ( !( createDomain.isSuccess() && alterDomain.isSuccess() ) ) {
            Assert.fail(
                    createDomain.getErrorMsg() + alterDomain.getErrorMsg() );
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
                opt.put( "AutoSplit", true );
                for ( int i = 0; i < 5; i++ ) {
                    db.createDomain(
                            domainName + "_" + random.nextInt( number ), opt );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -215 ) { // -215:Domain already exists
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.close();
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
                opt.put( "AutoSplit", false );
                for ( int i = 0; i < 5; i++ ) {
                    db.getDomain( domainName + "_" + random.nextInt( number ) )
                            .alterDomain( opt );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -214 ) { // -214:Domain does not exist
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.close();
            }
        }
    }
}