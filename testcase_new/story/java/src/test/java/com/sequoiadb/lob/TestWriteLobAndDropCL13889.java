package com.sequoiadb.lob;

import java.util.Random;

import org.bson.types.ObjectId;
import org.testng.Assert;

import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: TestWriteLobAndDropCL13889.java test content:when drop cl of write
 * lob testlink case:seqDB-13889
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @update [2017.12.21]
 * @version 1.00
 */
public class TestWriteLobAndDropCL13889 extends SdbTestBase {

    private String clName = "cl_lob13889";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private Random random = new Random();

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }
        createCL();
    }

    @Test
    public void testWriteLobAndDropCL() {
        PutLobsTask putLobsTask = new PutLobsTask();
        putLobsTask.start( 20 );
        dropCL();

        Assert.assertTrue( putLobsTask.isSuccess(), putLobsTask.getErrorMsg() );
        Assert.assertEquals( cs.isCollectionExist( clName ), false,
                "the cl must be drop!" );
    }

    @AfterClass
    public void tearDown() {
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        sdb.disconnect();
    }

    private class PutLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int writeLobSize = random.nextInt( 1024 * 1024 );
                byte[] wlobBuff = LobOprUtils.getRandomBytes( writeLobSize );
                ObjectId oid = LobOprUtils.createAndWriteLob( dbcl, wlobBuff );

                // read lob and check the lob data
                DBLob rLob = dbcl.openLob( oid );
                byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                rLob.read( rbuff );
                rLob.close();
                LobOprUtils.assertByteArrayEqual( rbuff, wlobBuff,
                        "lob data is wrong!the oid: " + oid.toString() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -317 && e.getErrorCode() != -23
                        && e.getErrorCode() != -4 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private void dropCL() {
        // random time to delete cl in writing lob
        long sleeptime = random.nextInt( 2000 );
        try {
            Thread.sleep( sleeptime );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        cs.dropCollection( clName );
    }

    private void createCL() {
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }
}
