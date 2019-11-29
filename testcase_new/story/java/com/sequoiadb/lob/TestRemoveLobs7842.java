package com.sequoiadb.lob;

import java.util.Random;

import org.bson.BSONObject;
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

/**
 * FileName: TestRemoveLobs7842.java test content:remove lobs testlink
 * case:seqDB-7842
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestRemoveLobs7842 extends SdbTestBase {

    private String clName = "cl_lob7842";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private Random random = new Random();
    private static final String LOB_OID = "Oid";
    private static final String LOB_SIZE = "Size";
    private static final String LOB_AVAILABLE = "Available";

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

    public void createCL() {
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
        }
    }

    @Test
    public void removeLob() {
        DBLob lob = null;
        ObjectId id = null;
        int lobsize = 0;
        try {
            lobsize = random.nextInt( 1048576 );
            String lobStringBuff = LobOprUtils.getRandomString( lobsize );
            lob = cl.createLob();
            lob.write( lobStringBuff.getBytes() );
            id = lob.getID();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } finally {
            if ( lob != null ) {
                lob.close();
            }
        }

        DBCursor listCursor = cl.listLobs();
        while ( listCursor.hasNext() ) {
            BSONObject obj = listCursor.getNext();
            ObjectId queryLobId = ( ObjectId ) obj.get( LOB_OID );
            Assert.assertEquals( true, queryLobId.equals( id ),
                    "query lob id different" );
            long actLobSize = ( Long ) obj.get( LOB_SIZE );
            boolean isAvailable = ( Boolean ) obj.get( LOB_AVAILABLE );
            Assert.assertEquals( true, isAvailable, "lob is not available" );
            Assert.assertEquals( lobsize, actLobSize,
                    "query lobSize different" );
        }
        cl.removeLob( id );
        // check the remove result
        DBCursor listCursor1 = cl.listLobs();
        Assert.assertEquals( listCursor1.hasNext(), false,
                "list lob not null" );
    }

}
