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
 * TestLink: seqDB-10199: concurrency[detachCL, drop mainCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class SubCL10199 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10199";
    private String clName = "cl10199";
    private String mCSName = csName + "_m";
    private String sCSName = csName + "_s";
    private String mCLName = clName + "_m";
    private String sCLName = clName + "_s";

    @BeforeClass
    public void setUp() {
        // start time
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // judge the mode or node number
            if ( CommLib.isStandAlone( sdb ) || MetaDataUtils.oneCataNode( sdb )
                    || MetaDataUtils.oneDataNode( sdb ) ) {
                throw new SkipException(
                        "The mode is standlone or one node, skip the testCase." );
            }
            MetaDataUtils.clearCS( sdb, csName );

            sdb.createCollectionSpace( mCSName );
            sdb.createCollectionSpace( sCSName );
            createMainCL( sdb );
            createSubCL( sdb );
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
    public void test() {

        AttachCL attachCL = new AttachCL();
        attachCL.start();

        DetachCL detachCL = new DetachCL();
        detachCL.start();

        if ( !( attachCL.isSuccess() && detachCL.isSuccess() ) ) {
            Assert.fail( attachCL.getErrorMsg() + detachCL.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class AttachCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                attachCL( db );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                db.close();
            }
        }
    }

    private class DetachCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection clDB = db.getCollectionSpace( mCSName )
                        .getCollection( mCLName );

                clDB.detachCollection( sCSName + "." + sCLName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -242 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    public void createMainCL( Sequoiadb sdb ) {
        try {
            CollectionSpace csDB = sdb.getCollectionSpace( mCSName );

            BSONObject mOpt = new BasicBSONObject();
            BSONObject mSubObj = new BasicBSONObject();
            mSubObj.put( "a", 1 );
            mOpt.put( "ShardingKey", mSubObj );
            mOpt.put( "ReplSize", 0 );
            mOpt.put( "IsMainCL", true );
            csDB.createCollection( mCLName, mOpt );
        } catch ( BaseException e ) {
            throw e;
        }
    }

    public void createSubCL( Sequoiadb sdb ) {
        try {
            CollectionSpace csDB = sdb.getCollectionSpace( sCSName );

            BSONObject sOpt = new BasicBSONObject();
            BSONObject sSubObj = new BasicBSONObject();
            sSubObj.put( "a", 1 );
            sOpt.put( "ShardingKey", sSubObj );
            sOpt.put( "ReplSize", 0 );
            csDB.createCollection( sCLName, sOpt );
        } catch ( BaseException e ) {
            throw e;
        }
    }

    public void attachCL( Sequoiadb sdb ) {
        try {
            DBCollection clDB = sdb.getCollectionSpace( mCSName )
                    .getCollection( mCLName );

            BSONObject options = new BasicBSONObject();
            BSONObject lowBoundObj = new BasicBSONObject();
            BSONObject upBoundObj = new BasicBSONObject();
            lowBoundObj.put( "a", 0 );
            upBoundObj.put( "a", 100 );
            options.put( "LowBound", lowBoundObj );
            options.put( "UpBound", upBoundObj );
            clDB.attachCollection( sCSName + "." + sCLName, options );
        } catch ( BaseException e ) {
            throw e;
        }
    }

}