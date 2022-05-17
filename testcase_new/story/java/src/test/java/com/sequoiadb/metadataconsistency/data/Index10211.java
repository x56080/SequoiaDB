package com.sequoiadb.metadataconsistency.data;

import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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

/**
 * TestLink: seqDB-10211: concurrency[createIndex]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class Index10211 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10211";
    private String clName = "cl10211";
    private String idxName = "idx";

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode or node number
        if ( CommLib.isStandAlone( sdb ) || MetaDataUtils.oneDataNode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone or one node, skip the testCase." );
        }
        MetaDataUtils.clearCS( sdb, csName );

        CollectionSpace csDB = sdb.createCollectionSpace( csName );
        csDB.createCollection( clName );
        MetaDataUtils.insertData( sdb, csName, clName );
    }

    @AfterClass
    public void tearDown() {
        try {
            // clear env
            MetaDataUtils.clearCS( sdb, csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor te = new ThreadExecutor();
        for ( int i = 0; i < 5; i++ ) {
            te.addWorker( new CreateIndex() );
        }
        te.run();

        // check results
        MetaDataUtils.checkIndex( csName, clName );
    }

    private class CreateIndex extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
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
            }
        }
    }

}