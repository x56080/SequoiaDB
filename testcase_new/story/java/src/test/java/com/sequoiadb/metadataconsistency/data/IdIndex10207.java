package com.sequoiadb.metadataconsistency.data;

import java.text.SimpleDateFormat;

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
 * TestLink: seqDB-10207: concurrency[createIdIndex, drop CS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class IdIndex10207 extends SdbTestBase {
    private SimpleDateFormat dateFm = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss" );
    private static Sequoiadb sdb = null;
    private String csName = "cs10207";
    private String clName = "cl10207";

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

            sdb.createCollectionSpace( csName );
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
        CreadeIdIndex creadeIdIndex = new CreadeIdIndex();
        creadeIdIndex.start();

        DropCS dropCS = new DropCS();
        dropCS.start();

        if ( !( creadeIdIndex.isSuccess() && dropCS.isSuccess() ) ) {
            Assert.fail( creadeIdIndex.getErrorMsg() + dropCS.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkIndex( csName, clName );
        MetaDataUtils.checkCSOfCatalog( csName );
    }

    private class CreadeIdIndex extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

                CollectionSpace csDB = db.getCollectionSpace( csName );
                if ( csDB != null ) {
                    DBCollection clDB = csDB.getCollection( clName );

                    BSONObject opt = new BasicBSONObject();
                    opt.put( "SortBufferSize", 128 );
                    if ( clDB != null ) {
                        clDB.createIdIndex( opt );
                    }
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 && eCode != -34 && eCode != -147
                        && eCode != -248 && eCode != -190 ) {
                    throw e;
                }
            } finally {
                db.disconnect();
            }
        }
    }

    private class DropCS extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.dropCollectionSpace( csName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            } finally {
                db.disconnect();
            }
        }
    }

    public void createCL( String csName ) {
        try {
            BSONObject opt = new BasicBSONObject();
            opt.put( "AutoIndexId", false );
            sdb.getCollectionSpace( csName ).createCollection( clName, opt );
        } catch ( BaseException e ) {
            throw e;
        }
    }

}