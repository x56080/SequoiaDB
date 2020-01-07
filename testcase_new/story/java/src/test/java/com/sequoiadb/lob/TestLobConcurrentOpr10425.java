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

import java.util.Iterator;
import java.util.Random;
import java.util.concurrent.LinkedBlockingQueue;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: TestLobConcurrentOpr10425.java test content:write and read lob
 * ,when remove lob testlink cases:seqDB-10425
 * 
 * @author wuyan
 * @Date 2016.10.9
 * @update [2017.12.20]
 * @version 1.00
 */
public class TestLobConcurrentOpr10425 extends SdbTestBase {
    private String clName = "cl_lob10425";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private Random random = new Random();
    private LinkedBlockingQueue< SaveOidAndMd5 > id2md5 = new LinkedBlockingQueue< SaveOidAndMd5 >();

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

        createCL();
        // write lob
        int lobtimes = 100;
        writeLobAndGetMd5( cl, lobtimes );
    }

    @Test
    public void testSplitAndWrite() {
        ReadLobsTask readLobsTask = new ReadLobsTask();
        WriteLobsTask writeLobsTask = new WriteLobsTask();
        RemoveLobsTask removeLobsTask = new RemoveLobsTask();

        readLobsTask.start( 100 );
        writeLobsTask.start( 50 );
        removeLobsTask.start( 80 );

        Assert.assertTrue( readLobsTask.isSuccess(),
                readLobsTask.getErrorMsg() );
        Assert.assertTrue( writeLobsTask.isSuccess(),
                writeLobsTask.getErrorMsg() );
        Assert.assertTrue( removeLobsTask.isSuccess(),
                removeLobsTask.getErrorMsg() );

        // check the write and remove result
        checkLobData();
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( null != sdb ) {
                sdb.disconnect();
            }
        }
    }

    private class RemoveLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException, InterruptedException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                SaveOidAndMd5 oidAndMd5 = id2md5.take();
                dbcl.removeLob( oidAndMd5.getOid() );
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
                db.setSessionAttr( ( BSONObject ) JSON
                        .parse( "{'PreferedInstance':'M'}" ) );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                SaveOidAndMd5 oidAndMd5 = id2md5.take();
                ObjectId oid = oidAndMd5.getOid();

                DBLob rLob = dbcl.openLob( oid );
                byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                rLob.read( rbuff );
                rLob.close();
                String curMd5 = LobOprUtils.getMd5( rbuff );
                String prevMd5 = oidAndMd5.getMd5();
                Assert.assertEquals( curMd5, prevMd5 );
                id2md5.offer( oidAndMd5 );
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private class WriteLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int lobtimes = 2;
                writeLobAndGetMd5( dbcl, lobtimes );
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private void checkLobData() {
        DBCursor listLob = null;
        try {
            listLob = cl.listLobs();
            while ( listLob.hasNext() ) {
                BasicBSONObject obj = ( BasicBSONObject ) listLob.getNext();
                ObjectId existOid = obj.getObjectId( "Oid" );

                DBLob rLob = cl.openLob( existOid );
                byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                rLob.read( rbuff );
                rLob.close();
                String curMd5 = LobOprUtils.getMd5( rbuff );
                String prevMd5 = getLobMd5ByOid( existOid );
                ;
                Assert.assertEquals( curMd5, prevMd5,
                        "the list oid:" + existOid.toString() );
            }
        } finally {
            if ( listLob != null ) {
                listLob.close();
            }
        }
        // the list lobnums must be equal with the nums of the id2mda,if id2md5
        // is not empty
        Assert.assertEquals( id2md5.isEmpty(), true,
                "the remaining " + id2md5.size() + " oids were not found!" );
    }

    // find the md5 from expected queue
    private String getLobMd5ByOid( ObjectId lobOid ) {
        Iterator< SaveOidAndMd5 > iterator = id2md5.iterator();
        boolean found = false;
        String findMd5 = "";
        while ( iterator.hasNext() ) {
            SaveOidAndMd5 current = iterator.next();
            ObjectId oid = current.getOid();
            if ( oid.equals( lobOid ) ) {
                findMd5 = current.getMd5();
                id2md5.remove( current );
                found = true;
                break;
            }
        }

        // if oid does not exist in the queue,than error
        if ( !found ) {
            throw new RuntimeException( "oid[" + lobOid + "] not found" );
        }
        return findMd5;
    }

    private class SaveOidAndMd5 {
        private ObjectId oid;
        private String md5;

        public SaveOidAndMd5( ObjectId oid, String md5 ) {
            this.oid = oid;
            this.md5 = md5;
        }

        public ObjectId getOid() {
            return oid;
        }

        public String getMd5() {
            return md5;
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
            id2md5.offer( new SaveOidAndMd5( oid, prevMd5 ) );
        }
    }

    private void createCL() {
        try {
            String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:4096,"
                    + "ReplSize:0,Compressed:true}";
            BSONObject options = ( BSONObject ) JSON.parse( clOptions );
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }
}
