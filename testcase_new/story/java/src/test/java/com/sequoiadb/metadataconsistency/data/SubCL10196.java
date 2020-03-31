package com.sequoiadb.metadataconsistency.data;

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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10196: concurrency[detachCL, drop subCS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class SubCL10196 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private String csName = "cs10196";
    private String clName = "cl10196";
    private String mCSName = csName + "_m";
    private String sCSName = csName + "_s";
    private String mCLName = clName + "_m";
    private String sCLName = clName + "_s";
    private Random random = new Random();
    private int msec = 100;

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
            attachCL( sdb );
            MetaDataUtils.insertData( sdb, mCSName, mCLName );
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

        DetachCL detachCL = new DetachCL();
        detachCL.start();

        DropSubCS dropSubCS = new DropSubCS();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        dropSubCS.start();

        if ( !( detachCL.isSuccess() && dropSubCS.isSuccess() ) ) {
            Assert.fail( detachCL.getErrorMsg() + dropSubCS.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class DetachCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

                CollectionSpace csDB = db.getCollectionSpace( mCSName );
                if ( csDB.isCollectionExist( mCLName ) ) {
                    csDB.getCollection( mCLName )
                            .detachCollection( sCSName + "." + sCLName );
                }
            } catch ( NullPointerException e ) {

            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    private class DropSubCS extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

                db.dropCollectionSpace( sCSName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 ) {
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