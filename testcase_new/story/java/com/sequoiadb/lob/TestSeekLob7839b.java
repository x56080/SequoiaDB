package com.sequoiadb.lob;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.BeforeClass;

import java.util.Arrays;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestSeekLob7839b.java test content:lob seek:seek read lob from more
 * than one group testlink case:seqDB-7839
 * 
 * @author wuyan
 * @Date 2017.12.19 *
 * @version 1.00
 */

public class TestSeekLob7839b extends SdbTestBase {
    private String clName = "cl_lob7839b";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private ObjectId oid = null;
    private byte[] wlobBuff = null;

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

        createCL();
        // write lob
        int writeLobSize = 1024 * 1024 * 2;
        wlobBuff = LobOprUtils.getRandomBytes( writeLobSize );
        oid = LobOprUtils.createAndWriteLob( cl, wlobBuff );
        splitCL();
    }

    @Test
    public void testSeekAndReadLob() {
        DBLob rLob = null;
        try {
            rLob = cl.openLob( oid );
            int readsize = 1024 * 1024 * 2;
            int offset = 1024 * 1024 * 1;
            byte[] rbuff = new byte[ readsize ];
            rLob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            rLob.read( rbuff );
            byte[] expBuff = Arrays.copyOfRange( wlobBuff, offset,
                    offset + readsize );
            LobOprUtils.assertByteArrayEqual( rbuff, expBuff,
                    "lob data is wrong!the oid: " + oid.toString() );

        } finally {
            if ( rLob != null ) {
                rLob.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( null != sdb ) {
                sdb.disconnect();
            }
        }
    }

    private void createCL() {
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            BSONObject options = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{a:1,b:-1},"
                            + "ShardingType:'hash',Partition:4096,ReplSize:0}" );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    private void splitCL() {
        String sourceRGName = "";
        String targetRGName = "";
        try {
            int percent = 50;
            sourceRGName = LobOprUtils.getSrcGroupName( sdb, SdbTestBase.csName,
                    clName );
            targetRGName = LobOprUtils.getSplitGroupName( sourceRGName );
            cl.split( sourceRGName, targetRGName, percent );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "split fail:" + e.getMessage() + "srcRGName:" + sourceRGName
                            + "\n tarRGName:" + targetRGName );
        }
    }

}
