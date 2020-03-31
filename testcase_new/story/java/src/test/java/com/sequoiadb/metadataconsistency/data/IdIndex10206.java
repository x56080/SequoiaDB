package com.sequoiadb.metadataconsistency.data;

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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10206: concurrency[attachCL, alter cl]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class IdIndex10206 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10206";
    private String clName = "cl10206";

    @BeforeClass
    public void setUp() {
        // start time
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // judge the mode or group number or node number
            if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb )
                    || MetaDataUtils.oneCataNode( sdb )
                    || MetaDataUtils.oneDataNode( sdb ) ) {
                throw new SkipException(
                        "The mode is standlone or only one group or one node, "
                                + "skip the testCase." );
            }
            MetaDataUtils.clearCS( sdb, csName );

            createCL( csName );
            MetaDataUtils.insertData( sdb, csName, clName );
        } catch ( BaseException e ) {
            sdb.close();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            MetaDataUtils.clearCS( sdb, csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() throws InterruptedException {
        CreateIdIndex createIndex = new CreateIdIndex();
        createIndex.start();

        AlterCL alterCL = new AlterCL();
        alterCL.start();

        if ( !( createIndex.isSuccess() && alterCL.isSuccess() ) ) {
            Assert.fail( createIndex.getErrorMsg() + alterCL.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkIndex( csName, clName );
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class CreateIdIndex extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( clName );

                BSONObject opt = new BasicBSONObject();
                opt.put( "SortBufferSize", 128 );
                clDB.createIdIndex( opt );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -43 && eCode != -108 ) { // -43:Failed to
                                                       // initialize index
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.close();
            }
        }
    }

    private class AlterCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( clName );

                BSONObject opt = new BasicBSONObject();
                opt.put( "ReplSize", 1 );
                clDB.alterCollection( opt );
            } catch ( BaseException e ) {
                Assert.fail( e.getMessage() );
            } finally {
                db.close();
            }
        }
    }

    public void createCL( String csName ) {
        try {
            CollectionSpace csDB = sdb.createCollectionSpace( csName );

            BSONObject opt = new BasicBSONObject();
            opt.put( "AutoIndexId", false );
            csDB.createCollection( clName, opt );
        } catch ( BaseException e ) {
            throw e;
        }
    }
}