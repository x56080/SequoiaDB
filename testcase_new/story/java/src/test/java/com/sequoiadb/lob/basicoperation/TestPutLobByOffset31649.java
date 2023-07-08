package com.sequoiadb.lob.basicoperation;

import java.util.Arrays;

import com.sequoiadb.exception.SDBError;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import org.bson.types.ObjectId;
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
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @version 1.0
 * @Description seqDB-31649:putLob接口指定位置偏移与长度指定记录oid
 * @Author TangTao
 * @Date 2023.05.17
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.17
 */

public class TestPutLobByOffset31649 extends SdbTestBase {
    private String clName = "writelob31649";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    byte[] testLobBuff = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
    }

    @Test
    public void testLob() {
        int writeLobSize = 1024 * 64;
        int offset = 1024 * 4;
        int length = 1024 * 8;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );

        // case 1: insert normal lob data
        putAndCheck( testLobBuff, 0, testLobBuff.length );
        putAndCheck( testLobBuff, offset, length );
        putAndCheckWithOutID( testLobBuff, offset, length );

        // case 2: offset < 0
        try {
            putAndCheck( testLobBuff, -1, length );
            Assert.fail( "putLob offset < 0, Expected to failed." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() )
                throw ( e );
        }
        // case 3: offset = buff length
        putAndCheck( testLobBuff, testLobBuff.length, 0 );

        // case 4: offset > buff length
        try {
            putAndCheck( testLobBuff, testLobBuff.length + 1, length );
            Assert.fail( "putLob offset > buff.length, Expected to failed." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() )
                throw ( e );
        }

        // case 5: len < 0
        try {
            putAndCheck( testLobBuff, offset, -1 );
            Assert.fail( "putLob len < 0, Expected to failed." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() )
                throw ( e );
        }

        // case 6: len = 0
        putAndCheck( testLobBuff, offset, 0 );

        // case 7: len > data length
        try {
            putAndCheck( testLobBuff, 0, testLobBuff.length + 1 );
            Assert.fail( "putLob len > buff.length, Expected to failed." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() )
                throw ( e );
        }

        // case 8: offset + len > data length
        try {
            putAndCheck( testLobBuff, offset, testLobBuff.length );
            Assert.fail(
                    "putLob offset+len > buff.length, Expected to failed." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() )
                throw ( e );
        }
    }

    @AfterClass
    public void tearDown() {
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private void putAndCheck( byte[] data, int offset, int len ) {
        ObjectId oid = cl.createLobID();
        cl.putLob( data, offset, len, oid );

        byte[] exceptData = Arrays.copyOfRange( data, offset, offset + len );
        checkLobData( oid, exceptData );
    }

    private void putAndCheckWithOutID( byte[] data, int offset, int len ) {
        ObjectId oid = null;
        cl.putLob( data, offset, len, oid );
    }

    private void checkLobData( ObjectId oid, byte[] data ) {
        byte[] actual = new byte[ data.length ];
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_READ )) {
            Assert.assertEquals( data.length, lob.getSize() );
            lob.read( actual );
            RandomWriteLobUtil.assertByteArrayEqual( data, actual );
        }
    }
}
