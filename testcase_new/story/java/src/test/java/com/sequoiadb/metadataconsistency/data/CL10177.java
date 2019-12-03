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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10171: concurrency[createCL, alterCL, dropCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class CL10177 extends SdbTestBase {
    private SimpleDateFormat dateFm = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss" );
    private static Sequoiadb sdb = null;
    private String csName = "cs10177";
    private String clName = "cl10177";
    private Random random = new Random();
    private int number = 20;
    private int msec = 100;

    @BeforeClass
    public void setUp() {
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
            createCL();
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

    @Test(invocationCount = 3, threadPoolSize = 3)
    public void test() {

        AlterCL alterCL = new AlterCL();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        alterCL.start();

        if ( !alterCL.isSuccess() ) {
            Assert.fail( alterCL.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class AlterCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            int rep = random.nextInt( 7 );
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                CollectionSpace csDB = db.getCollectionSpace( csName );

                BSONObject opt = new BasicBSONObject();
                opt.put( "ReplSize", rep );
                csDB.getCollection( clName + "_" + random.nextInt( number ) )
                        .alterCollection( opt );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 ) {
                    System.out.println( "ReplSize:" + rep );
                    throw e;
                }
            } finally {
                db.disconnect();
            }
        }
    }

    public void createCL() {
        try {
            CollectionSpace csDB = sdb.getCollectionSpace( csName );

            BSONObject opt = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "a", 1 );
            opt.put( "ShardingType", "hash" );
            opt.put( "ShardingKey", subObj );
            opt.put( "ReplSize", 0 );
            opt.put( "AutoSplit", true );
            for ( int i = 0; i < number; i++ ) {
                String tmpCLName = clName + "_" + i;
                csDB.createCollection( tmpCLName, opt );
                MetaDataUtils.insertData( sdb, csName, tmpCLName );
            }
        } catch ( BaseException e ) {
            throw e;
        }
    }

}