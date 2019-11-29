package com.sequoiadb.lob;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.SkipException;

import java.util.ArrayList;
import java.util.Random;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestLobAutoSplit7845.java test content:set AutoSplit of cs ,then
 * write lobs , testlink case:seqDB-7845
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @update [2017.12.20]
 * @version 1.00
 */
public class TestLobAutoSplit7845 extends SdbTestBase {

    private String domainName = "domain7845";
    private String csName = "cs_lob7845";
    private String clName = "cl_lob7845";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private Random random = new Random();
    private ArrayList< String > groupList;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        groupList = LobOprUtils.getDataGroups( sdb );
        createDomain( groupList.size() );
        createCSAndCLOfDomain();
    }

    // eg:with 30 threads to write 100 lob
    @Test(invocationCount = 100, threadPoolSize = 30)
    public void testAutoSplitLob() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection dbcl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            // write lob
            int lobsize = random.nextInt( 1024 * 1024 );
            byte[] wlobBuff = LobOprUtils.getRandomBytes( lobsize );
            ObjectId oid = LobOprUtils.createAndWriteLob( dbcl, wlobBuff );

            // read lob and check the lob data
            DBLob rLob = dbcl.openLob( oid );
            byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
            rLob.read( rbuff );
            rLob.close();
            LobOprUtils.assertByteArrayEqual( rbuff, wlobBuff,
                    "lob data is wrong!the oid: " + oid.toString() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    @Test(dependsOnMethods = "testAutoSplitLob")
    public void checkSplitResult() {
        double expErrorValue = 0.95;
        LobOprUtils.checkSplitResult( sdb, csName, clName, groupList,
                expErrorValue );
    }

    @AfterClass
    public void tearDown() {
        try {
            dropCSAndDomain();
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( null != sdb ) {
                sdb.disconnect();
            }
        }
    }

    private void createDomain( int groupsNum ) {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }

            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
            BSONObject options = new BasicBSONObject();
            options = ( BSONObject ) JSON.parse( "{'Groups': ["
                    + LobOprUtils.chooseDataGroups( sdb, groupsNum )
                    + "],AutoSplit:true}" );
            sdb.createDomain( domainName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "create domain fail:" + e.getErrorCode() + e.getMessage() );
        }
    }

    private void createCSAndCLOfDomain() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            BSONObject optionsCs = new BasicBSONObject();
            optionsCs = ( BSONObject ) JSON
                    .parse( "{Domain:'" + domainName + "'}" );
            cs = sdb.createCollectionSpace( csName, optionsCs );

            String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                    + "Compressed:true}";
            BSONObject options = ( BSONObject ) JSON.parse( clOptions );
            cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cs/cl fail:csName:" + csName
                    + e.getErrorCode() + e.getMessage() );
        }
    }

    public void dropCSAndDomain() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "drop domain/cs failed:"
                    + e.getErrorCode() + e.getMessage() );
        }
    }

}
