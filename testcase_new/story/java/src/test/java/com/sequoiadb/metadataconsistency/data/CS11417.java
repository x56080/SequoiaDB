package com.sequoiadb.metadataconsistency.data;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-11417: concurrency[dropCS the same cs]
 * 
 * @author xiaoni huang init
 * @Date 2016.9.25
 */

public class CS11417 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs11417";

    @BeforeClass
    public void setUp() {
        // start time
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // judge the mode or node number
            if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
                throw new SkipException(
                        "The mode is standlone or one node, skip the testCase." );
            }
            MetaDataUtils.clearCS( sdb, csName );
            this.createCS();
        } catch ( BaseException e ) {
            sdb.close();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            this.createCS();
            MetaDataUtils.clearCS( sdb, csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.close();
        }
    }

    @Test(invocationCount = 10, threadPoolSize = 10)
    public void test() {
        DropCS dropCS = new DropCS();
        dropCS.start();

        if ( !dropCS.isSuccess() ) {
            Assert.fail( dropCS.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkCSOfCatalog( csName );
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
                if ( eCode != -34 && eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    private void createCS() throws BaseException {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db.createCollectionSpace( csName );
        } catch ( BaseException e ) {
            throw e;
        } finally {
            db.close();
        }
    }

}