package com.sequoiadb.metadataconsistency.data;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10179: concurrency[alterCL, dropCS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class CL10179 extends SdbTestBase {
    private SimpleDateFormat dateFm = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss" );
    private static Sequoiadb sdb = null;
    private static ArrayList< String > dataGroups = null;
    private String domainName = "dm10179";
    private String csName = "cs10179";
    private String clName = "cl10179";
    private Random random = new Random();
    private int msec = 100;

    @BeforeClass
    public void setUp() {
        // start time
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // judge the mode or group number or node number
            if ( MetaDataUtils.isStandAlone( sdb )
                    || MetaDataUtils.OneGroupMode( sdb )
                    || MetaDataUtils.oneCataNode( sdb )
                    || MetaDataUtils.oneDataNode( sdb ) ) {
                throw new SkipException(
                        "The mode is standlone or only one group or one node, "
                                + "skip the testCase." );
            }
            MetaDataUtils.clearCS( sdb, csName );
            MetaDataUtils.clearDomain( sdb, domainName );

            dataGroups = MetaDataUtils.getDataGroupNames( sdb );

            createDomain();
            createCS();
            sdb.getCollectionSpace( csName ).createCollection( clName );
            MetaDataUtils.insertData( sdb, csName, clName );
        } catch ( BaseException e ) {
            sdb.disconnect();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            MetaDataUtils.clearCS( sdb, csName );
            MetaDataUtils.clearDomain( sdb, domainName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.disconnect();
        }
    }

    @Test
    public void test() {
        AlterCL alterCL = new AlterCL();
        alterCL.start();

        DropCS dropCS = new DropCS();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        dropCS.start();

        if ( !( alterCL.isSuccess() && dropCS.isSuccess() ) ) {
            Assert.fail( alterCL.getErrorMsg() + dropCS.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class AlterCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

                BSONObject opt = new BasicBSONObject();
                opt.put( "ReplSize", 7 );
                CollectionSpace csDB = db.getCollectionSpace( csName );
                if ( csDB != null ) {
                    csDB.getCollection( clName ).alterCollection( opt );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -34 && eCode != -23 && eCode != -147
                        && eCode != -248 && eCode != -190 ) {
                    throw e;
                }
            } finally {
                db.disconnect();
            }
        }
    }

    private class DropCS extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.dropCollectionSpace( csName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            } finally {
                db.disconnect();
            }
        }
    }

    public void createDomain() {
        try {
            BSONObject opt = new BasicBSONObject();
            opt.put( "Groups", dataGroups );
            opt.put( "AutoSplit", true );
            sdb.createDomain( domainName, opt );
        } catch ( BaseException e ) {
            throw e;
        }
    }

    public void createCS() {
        try {
            BSONObject opt = new BasicBSONObject();
            opt.put( "Domain", domainName );
            sdb.createCollectionSpace( csName, opt );
        } catch ( BaseException e ) {
            throw e;
        }
    }

}