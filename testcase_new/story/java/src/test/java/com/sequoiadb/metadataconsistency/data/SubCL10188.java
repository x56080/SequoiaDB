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
 * TestLink: seqDB-10188: concurrency[attachCL, drop subCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class SubCL10188 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private String csName = "cs10188";
    private String clName = "cl10188";
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

            sdb.createCollectionSpace( csName );
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

        DropSubCL dropSubCL = new DropSubCL();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        dropSubCL.start();

        if ( !( attachCL.isSuccess() && dropSubCL.isSuccess() ) ) {
            Assert.fail( attachCL.getErrorMsg() + dropSubCL.getErrorMsg() );
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

                BSONObject options = new BasicBSONObject();
                BSONObject lowBoundObj = new BasicBSONObject();
                BSONObject upBoundObj = new BasicBSONObject();
                lowBoundObj.put( "a", 0 );
                upBoundObj.put( "a", 200 );
                options.put( "LowBound", lowBoundObj );
                options.put( "UpBound", upBoundObj );
                CollectionSpace csDB = db.getCollectionSpace( csName );
                if ( csDB.isCollectionExist( mCLName ) ) {
                    DBCollection clDB = csDB.getCollection( mCLName );
                    clDB.attachCollection( csName + "." + sCLName, options );

                    MetaDataUtils.insertData( db, csName, mCLName );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 && eCode != -34 && eCode != -135 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    private class DropSubCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                CollectionSpace csDB = db.getCollectionSpace( csName );

                csDB.dropCollection( sCLName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    public void createMainCL( Sequoiadb sdb ) {
        try {
            CollectionSpace csDB = sdb.getCollectionSpace( csName );

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
            CollectionSpace csDB = sdb.getCollectionSpace( csName );

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

}