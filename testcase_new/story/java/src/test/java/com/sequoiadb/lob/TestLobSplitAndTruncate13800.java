package com.sequoiadb.lob;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: TestLobSplitAndTruncate13800.java test content:lob split and
 * truncate testlink case:seqDB-13800
 * 
 * @author luweikang
 * @Date 2017.12.19
 * @version 1.00
 */
public class TestLobSplitAndTruncate13800 extends SdbTestBase {

    private String clName = "split_truncate13800";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        createCL( sdb );
        putLob( cl );
    }

    @Test
    public void spiltAndTruncate() throws InterruptedException {
        Split split = new Split();
        split.start();

        Thread.sleep( 5000 );

        try {
            cl.truncate();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "truncate cl failed:" + e.getMessage() );
        }

        if ( cl.listLobs().hasNext() ) {
            Assert.assertTrue( false, "cl should be empty!" );
        }

        if ( !split.isSuccess() ) {
            Assert.assertTrue( false, "split cl failed!" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( null != sdb ) {
                sdb.disconnect();
            }
        }
    }

    public void createCL( Sequoiadb db ) {
        cs = db.getCollectionSpace( SdbTestBase.csName );
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            BSONObject options = new BasicBSONObject();
            options = ( BSONObject ) JSON.parse(
                    "{ShardingKey:{a:1},ShardingType:'hash',Partition:4096}" );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl failed:" + e.getMessage() );
        }
    }

    public void putLob( DBCollection cl ) {
        long lobNums = 30;
        for ( long i = 0; i < lobNums; i++ ) {
            String lobSb = LobOprUtils.getRandomString( 1024 * 1024 * 20 );
            DBLob lob = null;
            try {
                lob = cl.createLob();
                lob.write( lobSb.getBytes() );
            } catch ( BaseException e ) {
                Assert.assertTrue( false, "write lob fail:" + e.getMessage()
                        + e.getStackTrace() );
            } finally {
                if ( lob != null ) {
                    lob.close();
                }
            }
        }
    }

    private class Split extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                String sourceRGName = LobOprUtils.getSrcGroupName( db,
                        SdbTestBase.csName, clName );
                String targetRGName = LobOprUtils
                        .getSplitGroupName( sourceRGName );
                cl.split( sourceRGName, targetRGName, 50 );
            } catch ( BaseException e ) {
                Assert.assertTrue( false,
                        "split cl fail:" + e.getMessage() + e.getStackTrace() );
            } finally {
                db.disconnect();
            }
        }
    }
}
