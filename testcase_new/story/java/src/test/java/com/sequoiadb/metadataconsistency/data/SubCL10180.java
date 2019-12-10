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
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10180: concurrency[alter subCL, drop mainCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class SubCL10180 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private String csName = "cs10180";
    private String clName = "cl10180";
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
            if ( MetaDataUtils.isStandAlone( sdb )
                    || MetaDataUtils.oneCataNode( sdb )
                    || MetaDataUtils.oneDataNode( sdb ) ) {
                throw new SkipException(
                        "The mode is standlone or one node, skip the testCase." );
            }
            MetaDataUtils.clearCS( sdb, csName );

            sdb.createCollectionSpace( csName );
            createMainCL( sdb );
            createSubCL( sdb );
            attachCL( sdb );
            MetaDataUtils.insertData( sdb, csName, mCLName );
        } catch ( BaseException e ) {
            sdb.disconnect();
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
            sdb.disconnect();
        }
    }

    @Test
    public void test() {

        AlterSubCL alterSubCL = new AlterSubCL();
        alterSubCL.start();

        DropMainCL dropMainCL = new DropMainCL();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        dropMainCL.start();

        if ( !( alterSubCL.isSuccess() && dropMainCL.isSuccess() ) ) {
            Assert.fail( alterSubCL.getErrorMsg() + dropMainCL.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class AlterSubCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                CollectionSpace csDB = db.getCollectionSpace( csName );

                if ( csDB.isCollectionExist( sCLName ) ) {
                    BSONObject opt = new BasicBSONObject();
                    opt.put( "ReplSize", 7 );
                    csDB.getCollection( sCLName ).alterCollection( opt );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 && eCode != -108 ) {
                    throw e;
                }
            } finally {
                db.disconnect();
            }
        }
    }

    private class DropMainCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.getCollectionSpace( csName ).dropCollection( mCLName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 ) { // -147:Unable to lock
                    throw e;
                }
            } finally {
                db.disconnect();
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

    public void attachCL( Sequoiadb sdb ) {
        try {
            CollectionSpace csDB = sdb.getCollectionSpace( csName );

            BSONObject opt = new BasicBSONObject();
            BSONObject lowBound = new BasicBSONObject();
            BSONObject upBound = new BasicBSONObject();
            lowBound.put( "a", 0 );
            upBound.put( "a", 200 );
            opt.put( "LowBound", lowBound );
            opt.put( "UpBound", upBound );
            if ( csDB.isCollectionExist( sCLName ) ) {
                DBCollection clDB = csDB.getCollection( mCLName );
                clDB.attachCollection( csName + "." + sCLName, opt );
            }
        } catch ( BaseException e ) {
            throw e;
        }
    }

}