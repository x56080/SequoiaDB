package com.sequoiadb.lob;

import java.nio.ByteBuffer;

import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;

import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: TestReadAndRemoveLobs7843.java test content:when delete lob of
 * reading testlink case:seqDB-7843
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestReadAndRemoveSameLob7843 extends SdbTestBase {

    private String clName = "cl_lob7843";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private static ObjectId oid = null;
    private String prevMd5 = "";
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
    public void testSplitAndWrite() {
        putLob();
        ReadLob readLob = new ReadLob();
        readLob.start();
        removeLob();
        if ( !readLob.isSuccess() ) {
            Assert.fail( readLob.getErrorMsg() );
        }
    }

    public class ReadLob extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db2 = null;
            try {
                db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                String curMd5 = "";
                DBLob rLob = null;
                rLob = cl2.openLob( oid );
                byte[] rbuff = new byte[ 1024 ];
                int readLen = 0;
                ByteBuffer bytebuff = ByteBuffer
                        .allocate( ( int ) rLob.getSize() );
                while ( ( readLen = rLob.read( rbuff ) ) != -1 ) {
                    bytebuff.put( rbuff, 0, readLen );
                }
                bytebuff.rewind();
                rLob.close();
                curMd5 = LobOprUtils.getMd5( bytebuff );
                Assert.assertEquals( curMd5, prevMd5,
                        "the lobs md5 different" );

            } catch ( BaseException e ) {
                if ( -4 != e.getErrorCode() && -317 != e.getErrorCode()
                        && -268 != e.getErrorCode()
                        && -269 != e.getErrorCode() ) {
                    Assert.assertTrue( false, "removeLob fail:" + e.getMessage()
                            + e.getErrorCode() );
                }
            } finally {
                if ( db2 != null ) {
                    db2.disconnect();
                }
            }
        }
    }

    public void removeLob() {
        Sequoiadb db1 = null;
        try {
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            cl1.removeLob( oid );
            boolean IsNotExist = true;
            DBCursor cur1 = cl.listLobs();
            while ( cur1.hasNext() ) {
                BasicBSONObject obj = ( BasicBSONObject ) cur1.getNext();
                if ( obj.getObjectId( "Oid" ).equals( oid ) ) {
                    IsNotExist = false;
                    break;
                }
            }
            cur1.close();
            Assert.assertTrue( IsNotExist, "lob remove fail" );
        } catch ( BaseException e ) {
            if ( -4 != e.getErrorCode() && -317 != e.getErrorCode()
                    && -268 != e.getErrorCode() && -269 != e.getErrorCode() ) {
                Assert.assertTrue( false,
                        "removeLob fail:" + e.getMessage() + e.getErrorCode() );
            }
        } finally {
            if ( db1 != null ) {
                db1.disconnect();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    private void putLob() {
        int lobsize = random.nextInt( 1048576 );
        String lobSb = LobOprUtils.getRandomString( lobsize );
        DBLob lob = null;
        try {
            lob = cl.createLob();
            lob.write( lobSb.getBytes() );
            oid = lob.getID();
            prevMd5 = LobOprUtils.getMd5( lobSb );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "write lob fail" + e.getMessage() );
        } finally {
            if ( lob != null ) {
                lob.close();
            }
        }
    }

    public void createCL() {
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
    }
}
