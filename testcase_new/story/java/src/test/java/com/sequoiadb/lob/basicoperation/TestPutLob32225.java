package com.sequoiadb.lob.basicoperation;

import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @version 1.0
 * @Description seqDB-32225:putLob接口写入小文件lob
 * @Author TangTao
 * @Date 2023.06.15
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.06.15
 */

public class TestPutLob32225 extends SdbTestBase {
    private String clName = "writelob32225";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    byte[] testLobBuff = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
    }

    @Test
    public void testLob() {

        // case 1: 插入数据为空
        int writeLobSize = 0;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        putAndCheck( testLobBuff );

        // case 2: 插入数据小于255KB
        writeLobSize = 1024 * 64;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        putAndCheck( testLobBuff );

        // case 3: 插入数据等于于255KB
        writeLobSize = 1024 * 255;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        putAndCheck( testLobBuff );

        // case 4: 插入数据大于255KB
        writeLobSize = 1024 * 256;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        putAndCheck( testLobBuff );

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

    private void putAndCheck( byte[] data ) {
        ObjectId oid = null;
        oid = cl.putLob( data );
        checkLobData( oid, data );
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
