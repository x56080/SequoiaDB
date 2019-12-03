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
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10182: concurrency[split]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.24
 */

public class Split10182 extends SdbTestBase {
    private SimpleDateFormat dateFm = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss" );
    private static Sequoiadb sdb = null;
    private static ArrayList< String > groupNames = null;
    private String csName = "cs10182";
    private String clName = "cl10182";
    private Random random = new Random();
    private int msec = 500;

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

            groupNames = MetaDataUtils.getDataGroupNames( sdb );

            sdb.createCollectionSpace( csName );
            createCL( sdb, groupNames.get( 0 ) );
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
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.disconnect();
        }
    }

    @Test
    public void test() {

        Split split = new Split();
        split.start();

        MetaDataUtils.sleep( random.nextInt( msec ) );
        split.start();

        if ( !split.isSuccess() ) {
            Assert.fail( split.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class Split extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( clName );

                BSONObject strCond = new BasicBSONObject();
                BSONObject endCond = new BasicBSONObject();
                Random i = new Random();
                int bound = i.nextInt( 10000 );
                strCond.put( "a", bound );
                endCond.put( "a", bound + 100 );
                clDB.split( groupNames.get( 0 ), groupNames.get( 1 ), strCond,
                        endCond );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode(); // -176:SDB_CLS_BAD_SPLIT_KEY,split
                                              // bound exist
                if ( eCode != -175 && eCode != -147 && eCode != -176
                        && eCode != -190 ) { // -175:The
                                             // mutex
                                             // task
                                             // already
                                             // exist
                    throw e;
                }
            } finally {
                db.disconnect();
            }
        }
    }

    private void createCL( Sequoiadb sdb, String rgName ) {
        try {
            CollectionSpace csDB = sdb.getCollectionSpace( csName );
            BSONObject opt = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "a", 1 );
            opt.put( "ShardingType", "range" );
            opt.put( "ShardingKey", subObj );
            opt.put( "Group", rgName );
            opt.put( "ReplSize", 0 );
            csDB.createCollection( clName, opt );
        } catch ( BaseException e ) {
            throw e;
        }
    }

}