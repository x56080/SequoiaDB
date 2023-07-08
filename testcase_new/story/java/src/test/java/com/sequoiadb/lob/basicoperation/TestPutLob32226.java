package com.sequoiadb.lob.basicoperation;

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @version 1.0
 * @Description seqDB-32226:putLob接口指定不同oid写入小文件lob
 * @Author TangTao
 * @Date 2023.06.15
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.06.15
 */

public class TestPutLob32226 extends SdbTestBase {
    private String clName = "writelob32226";
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
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );

        // case 1: putLob data为空，也不指定oid
        byte[] testLobBuff2 = RandomWriteLobUtil.getRandomBytes( 0 );
        cl.putLob( testLobBuff2, null );

        // case 2: putLob 指定data，不指定oid/oid为null
        cl.putLob( testLobBuff );
        cl.putLob( testLobBuff, null );

        // case 3: putLob 指定data，oid为新建的oid
        ObjectId oid = cl.createLobID();
        cl.putLob( testLobBuff, oid );

        // case 4: putLob 指定data，oid为普通ObjectID
        try {
            ObjectId oid1 = new ObjectId( 0, 0, 0 );
            cl.putLob( testLobBuff, oid1 );
            Assert.fail( "putLob oid was null, need to throw error" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        // case 5: putLob 指定data，oid为已存在的oid
        try {
            cl.putLob( testLobBuff, oid );
            Assert.fail( "oid is already exists, need to throw error" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_FE.getErrorCode() ) {
                throw e;
            }
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
}
