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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10172: create autoSplit collection seqDB-10178:
 * concurrency[alterCL, dropCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class CL10178 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private static ArrayList< String > dataGroups = null;
    private String domainName = "dm10178";
    private String csName = "cs10178";
    private String clName = "cl10178";
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
            createCL();
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

    @Test(invocationCount = 10, threadPoolSize = 10)
    public void test() {
        AlterCL alterCL = new AlterCL();
        alterCL.start();

        DropCL dropCL = new DropCL();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        dropCL.start();

        if ( !( alterCL.isSuccess() && dropCL.isSuccess() ) ) {
            Assert.fail( alterCL.getErrorMsg() + dropCL.getErrorMsg() );
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
                CollectionSpace csDB = db.getCollectionSpace( csName );

                BSONObject opt = new BasicBSONObject();
                opt.put( "ReplSize", 2 );
                DBCollection clDB = csDB.getCollection( clName );
                if ( clDB != null ) {
                    clDB.alterCollection( opt );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -23 && eCode != -108
                        && eCode != -190 ) {
                    throw e;
                }
            } finally {
                db.disconnect();
            }
        }
    }

    private class DropCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                CollectionSpace csDB = db.getCollectionSpace( csName );

                csDB.dropCollection( clName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode == -147 && eCode != -190 ) {
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

    public void createCL() {
        try {
            CollectionSpace csDB = sdb.getCollectionSpace( csName );

            BSONObject opt = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "a", 1 );
            opt.put( "ShardingType", "hash" );
            opt.put( "ShardingKey", subObj );
            opt.put( "ReplSize", 1 );
            opt.put( "AutoSplit", true );
            csDB.createCollection( clName, opt );
        } catch ( BaseException e ) {
            throw e;
        }
    }

}