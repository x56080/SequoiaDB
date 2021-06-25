package com.sequoiadb.split;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Random;
import java.util.concurrent.LinkedBlockingQueue;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-24278:100%切分和putLob并发
 * @author liuli
 * @Date 2021.06.25
 * @version 1.10
 */
public class Split24278 extends SdbTestBase {
    private String csName = "cs_24278";
    private String clName = "cl_24278";
    private DBCollection dbcl;
    List< String > groupNames = new ArrayList<>();
    private Sequoiadb sdb = null;
    private Random random = new Random();
    private LinkedBlockingQueue< SaveOidAndMd5 > id2md5 = new LinkedBlockingQueue< SaveOidAndMd5 >();

    @BeforeClass()
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 3 ) {
            throw new SkipException(
                    "current environment less than three groups " );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", groupNames.get( 0 ) );
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        dbcl = dbcs.createCollection( clName, option );
        // 写入待切分的记录（0-10000)
        insertData( dbcl );
    }

    // 100%切分同时执行putLob
    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        Split split = new Split();
        PutLob putlob = new PutLob();
        es.addWorker( split );
        es.addWorker( putlob );
        es.run();

        Assert.assertEquals( split.getRetCode(), 0 );
        Assert.assertEquals( putlob.getRetCode(), 0 );
        checkLobData();
        checkDataSplitResult();
    }

    @AfterClass()
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class Split extends ResultStore {
        @ExecuteOrder(step = 1)
        public void split() throws InterruptedException {
            dbcl.split( groupNames.get( 0 ), groupNames.get( 2 ), 100 );
        }
    }

    private class PutLob extends ResultStore {
        @ExecuteOrder(step = 1)
        public void putLob()
                throws InterruptedException, NoSuchAlgorithmException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 20; i++ ) {
                    int writeLobSize = random.nextInt( 1024 * 1024 );
                    byte[] wlobBuff = getRandomBytes( writeLobSize );
                    ObjectId oid = createAndWriteLob( dbcl, wlobBuff );

                    // save oid and md5
                    String prevMd5 = getMd5( wlobBuff );
                    id2md5.offer( new SaveOidAndMd5( oid, prevMd5 ) );
                }
            }
        }
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

    private void insertData( DBCollection cl ) {
        List< BSONObject > insertRecords = new ArrayList< BSONObject >();
        for ( int i = 0; i < 10000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "_id", i );
            obj.put( "a", i );
            obj.put( "num", i );
            insertRecords.add( obj );
        }
        dbcl.insert( insertRecords );
    }

    private void checkLobData() throws NoSuchAlgorithmException {
        try ( DBCursor listLob = dbcl.listLobs()) {
            while ( listLob.hasNext() ) {
                BasicBSONObject obj = ( BasicBSONObject ) listLob.getNext();
                ObjectId existOid = obj.getObjectId( "Oid" );
                try ( DBLob rLob = dbcl.openLob( existOid )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    String curMd5 = getMd5( rbuff );
                    String prevMd5 = getLobMd5ByOid( existOid );
                    Assert.assertEquals( curMd5, prevMd5,
                            "the list oid:" + existOid.toString() );
                }
            }
        }
        // the list lobnums must be equal with the nums of the id2mda,if id2md5
        // is not empty
        Assert.assertEquals( id2md5.isEmpty(), true,
                "the remaining " + id2md5.size() + " oids were not found!" );
    }

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

    public static String getMd5( Object inbuff )
            throws NoSuchAlgorithmException {
        MessageDigest md5 = null;
        String value = "";

        md5 = MessageDigest.getInstance( "MD5" );
        if ( inbuff instanceof ByteBuffer ) {
            md5.update( ( ByteBuffer ) inbuff );
        } else if ( inbuff instanceof String ) {
            md5.update( ( ( String ) inbuff ).getBytes() );
        } else if ( inbuff instanceof byte[] ) {
            md5.update( ( byte[] ) inbuff );
        } else {
            Assert.fail( "invalid parameter!" );
        }
        BigInteger bi = new BigInteger( 1, md5.digest() );
        value = bi.toString( 16 );
        
        return value;
    }

    public static byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    private ObjectId createAndWriteLob( DBCollection dbcl, byte[] data ) {
        ObjectId lobId = dbcl.createLobID();
        DBLob lob = dbcl.createLob( lobId );
        lob.write( data );
        lob.close();
        return lob.getID();
    }

    private void checkDataSplitResult() {
        Node node = sdb.getReplicaGroup( groupNames.get( 0 ) ).getMaster();
        String hoseName = node.getHostName();
        int port = node.getPort();
        try ( Sequoiadb data = new Sequoiadb( hoseName, port, "", "" ) ;) {
            data.getCollectionSpace( csName ).getCollection( clName );
            Assert.fail( "expected failure, actual succcess" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST.getErrorCode()
                    && e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode() ) {
                throw e;
            }
        }
    }

}
