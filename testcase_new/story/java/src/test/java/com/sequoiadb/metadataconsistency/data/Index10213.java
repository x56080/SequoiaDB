package com.sequoiadb.metadataconsistency.data;

import java.text.SimpleDateFormat;
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
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10213: concurrency[createIndex in mainCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.20
 */

public class Index10213 extends SdbTestBase {
    private SimpleDateFormat dateFm = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss" );
    private static Sequoiadb sdb = null;
    private String csName = "cs10213";
    private String clName = "cs10213";
    private String mCLName = clName + "_m";
    private String sCLName = clName + "_s";
    private String idxName = "idx";
    private Random random = new Random();
    private int msec = 100;

    @BeforeClass
    public void setUp() {
        // start time
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // judge the mode or node number
            if ( MetaDataUtils.isStandAlone( sdb )
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
        CreateIndex createIndex = new CreateIndex();
        createIndex.start();

        MetaDataUtils.sleep( random.nextInt( msec ) );
        createIndex.start();

        if ( !createIndex.isSuccess() ) {
            Assert.fail( createIndex.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkIndex( csName, sCLName );
    }

    private class CreateIndex extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( mCLName );

                String name = idxName;
                Random i = new Random();
                BSONObject opt = new BasicBSONObject();
                opt.put( "a" + i.nextInt( 10000 ), 1 );
                clDB.createIndex( name + i.nextInt( 10000 ), opt, false,
                        false );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -247 // -247:Redefine index
                        && eCode != -291 ) { // -291:Exist one index which can
                                             // cover this scene
                    throw e;
                }
            } finally {
                db.disconnect();
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
            sdb.getCollectionSpace( csName ).createCollection( mCLName, opt );
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
            for ( int i = 0; i < 10; i++ ) {
                sdb.getCollectionSpace( csName ).createCollection( sCLName + i,
                        opt );
            }
        } catch ( BaseException e ) {
            throw e;
        }

    }

    public void attachCL( Sequoiadb sdb ) {
        try {
            BSONObject options = new BasicBSONObject();
            BSONObject lowBoundObj = new BasicBSONObject();
            BSONObject upBoundObj = new BasicBSONObject();
            for ( int i = 0; i < 10; i++ ) {
                int bound = i * 100;
                lowBoundObj.put( "a", bound );
                upBoundObj.put( "a", bound + 100 );
                options.put( "LowBound", lowBoundObj );
                options.put( "UpBound", upBoundObj );
                sdb.getCollectionSpace( csName ).getCollection( mCLName )
                        .attachCollection( csName + "." + sCLName + i,
                                options );
            }
        } catch ( BaseException e ) {
            throw e;
        }
    }
}