package com.sequoiadb.metadataconsistency.data;

import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10168: concurrency[drop cs of subCL, drop mainCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class CS10168 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10168";
    private String clName = "cl10168";
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
            // judge the mode or group number or node number
            if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb )
                    || MetaDataUtils.oneCataNode( sdb )
                    || MetaDataUtils.oneDataNode( sdb ) ) {
                throw new SkipException(
                        "The mode is standlone or only one group or one node, "
                                + "skip the testCase." );
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
            throw e;
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        DropMainCS dropMainCS = new DropMainCS();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        dropMainCS.start();

        DropSubCS dropSubCS = new DropSubCS();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        dropSubCS.start();

        if ( !( dropMainCS.isSuccess() && dropSubCS.isSuccess() ) ) {
            Assert.fail( dropMainCS.getErrorMsg() + dropSubCS.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkCSOfCatalog( csName );
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class DropSubCS extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.dropCollectionSpace( sCSName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    private class DropMainCS extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.getCollectionSpace( mCSName ).dropCollection( mCLName );
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
            BSONObject opt = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "a", 1 );
            opt.put( "ShardingKey", subObj );
            opt.put( "ReplSize", 0 );
            opt.put( "IsMainCL", true );
            sdb.getCollectionSpace( mCSName ).createCollection( mCLName, opt );
        } catch ( BaseException e ) {
            throw e;
        }
    }

    public void createSubCL( Sequoiadb sdb ) {
        try {
            BSONObject opt = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "a", 1 );
            opt.put( "ShardingKey", subObj );
            opt.put( "ReplSize", 0 );
            sdb.getCollectionSpace( sCSName ).createCollection( sCLName, opt );
        } catch ( BaseException e ) {
            throw e;
        }
    }

    public void attachCL( Sequoiadb sdb ) {
        try {
            DBCollection clDB = sdb.getCollectionSpace( mCSName )
                    .getCollection( mCLName );

            BSONObject opt = new BasicBSONObject();
            BSONObject lowBound = new BasicBSONObject();
            BSONObject upBound = new BasicBSONObject();
            lowBound.put( "a", 0 );
            upBound.put( "a", 100 );
            opt.put( "LowBound", lowBound );
            opt.put( "UpBound", upBound );
            clDB.attachCollection( sCSName + "." + sCLName, opt );
        } catch ( BaseException e ) {
            throw e;
        }
    }

}