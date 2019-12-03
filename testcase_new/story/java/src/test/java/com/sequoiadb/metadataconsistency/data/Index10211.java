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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10211: concurrency[createIndex]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class Index10211 extends SdbTestBase {
    private SimpleDateFormat dateFm = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss" );
    private static Sequoiadb sdb = null;
    private String csName = "cs10211";
    private String clName = "cl10211";
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

            CollectionSpace csDB = sdb.createCollectionSpace( csName );
            csDB.createCollection( clName );
            MetaDataUtils.insertData( sdb, csName, clName );
        } catch ( BaseException e ) {
            sdb.disconnect();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            // check results
            MetaDataUtils.checkIndex( csName, clName );

            // clear env
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
        MetaDataUtils.checkIndex( csName, clName );
    }

    private class CreateIndex extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( clName );

                BSONObject opt = new BasicBSONObject();
                opt.put( "a", 1 );
                clDB.createIndex( idxName, opt, false, false );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -247 && eCode != -147// -247:Redefine index
                        && eCode != -43 && eCode != -190 ) { // -43:Failed to
                                                             // initialize index
                    throw e;
                }
            } finally {
                db.disconnect();
            }
        }
    }

}