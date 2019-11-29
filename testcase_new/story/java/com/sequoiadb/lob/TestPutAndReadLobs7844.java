package com.sequoiadb.lob;

import java.util.Random;
import java.util.concurrent.LinkedBlockingDeque;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
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
 * FileName: TestPutAndReadLobs7844.java test content:when write lob of reading
 * testlink case:seqDB-7844
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @update 2017.12.19
 * @version 1.00
 */
public class TestPutAndReadLobs7844 extends SdbTestBase {
    private String clName = "cl_lob7844";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    private Random random = new Random();

    class LobInfo {
        public ObjectId oid;
        public String md5;
    }

    private LinkedBlockingDeque< LobInfo > lobInfoQue = new LinkedBlockingDeque< LobInfo >();

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        DBCollection cl = createCL();
        int lobtimes = 30;
        writeLobAndGetMd5( cl, lobtimes );
    }

    @Test
    public void testPutAndReadLob() {
        PutLobsTask putLobsTask = new PutLobsTask();
        putLobsTask.start( 30 );

        ReadLobsTask readLobsTask = new ReadLobsTask();
        readLobsTask.start( 60 );

        Assert.assertTrue( putLobsTask.isSuccess(), putLobsTask.getErrorMsg() );
        Assert.assertTrue( readLobsTask.isSuccess(),
                readLobsTask.getErrorMsg() );
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

    public DBCollection createCL() {
        DBCollection cl = null;
        try {
            String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                    + "ReplSize:0,Compressed:true}";
            BSONObject options = ( BSONObject ) JSON.parse( clOptions );

            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
        return cl;
    }

    private class PutLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );

                int lobtimes = 1;
                writeLobAndGetMd5( dbcl, lobtimes );
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private class ReadLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException, InterruptedException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                LobInfo lobinfotmp = lobInfoQue.take();
                ObjectId oid = lobinfotmp.oid;
                DBLob rLob = dbcl.openLob( oid );
                byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                rLob.read( rbuff );
                String curMd5 = LobOprUtils.getMd5( rbuff );
                String prevMd5 = lobinfotmp.md5;

                rLob.close();
                Assert.assertEquals( curMd5, prevMd5 );
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private void writeLobAndGetMd5( DBCollection cl, int lobtimes ) {
        for ( int i = 0; i < lobtimes; i++ ) {
            int writeLobSize = random.nextInt( 1024 * 1024 );
            ;
            byte[] wlobBuff = LobOprUtils.getRandomBytes( writeLobSize );
            ObjectId oid = LobOprUtils.createAndWriteLob( cl, wlobBuff );

            // save oid and md5
            String prevMd5 = LobOprUtils.getMd5( wlobBuff );
            // oidQueue.offer(oid);
            LobInfo lobInfoTmp = new LobInfo();
            lobInfoTmp.oid = oid;
            lobInfoTmp.md5 = prevMd5;
            // id2md5.put(oid, prevMd5);
            lobInfoQue.offer( lobInfoTmp );

        }
    }

}
