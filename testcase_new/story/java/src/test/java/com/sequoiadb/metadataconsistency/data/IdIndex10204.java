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
 * TestLink: seqDB-10204: concurrency[create IdIndex]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class IdIndex10204 extends SdbTestBase {
    private SimpleDateFormat dateFm = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss" );
    private static Sequoiadb sdb = null;
    private String csName = "cs10204";
    private String clName = "cl10204";
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

            createCL( csName );
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
        CreateIdIndex createIdIndex = new CreateIdIndex();
        createIdIndex.start();

        MetaDataUtils.sleep( random.nextInt( msec ) );
        createIdIndex.start();

        if ( !createIdIndex.isSuccess() ) {
            Assert.fail( createIdIndex.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkIndex( csName, clName );
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
                if ( eCode != -147 && eCode != -247 && eCode != -190 ) {
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.disconnect();
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