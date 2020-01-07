package com.sequoiadb.bsontypes;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MinKey;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: MinKeyTest10350.java* test interface: MinKey(),toString ()
 * TestLink: seqDB-10350:
 * 
 * @author wuyan
 * @Date 2016.10.14
 * @version 1.00
 */
public class MinKeyTest10350 extends SdbTestBase {

    private String clName = "cl_10350";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;
    private SimpleDateFormat sdf = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss.SSS" );

    @BeforeClass
    public void setUp() {
        System.out.println( this.getClass().getName() + " begin at "
                + sdf.format( new Date() ) );
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        createCL();
    }

    private void createCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }

        String test = "{ReplSize:0,Compressed:true}";
        BSONObject options = ( BSONObject ) JSON.parse( test );
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    @Test
    public void testGetDateAndToString() {
        try {
            BSONObject obj = new BasicBSONObject();
            MinKey minkey = new MinKey();
            obj.put( "minkey", minkey );
            cl.insert( obj );

            // check the insert result
            BSONObject tmp = new BasicBSONObject();
            DBCursor tmpCursor = cl.query( tmp, null, null, null );
            BasicBSONObject actRecs = null;
            while ( tmpCursor.hasNext() ) {
                actRecs = ( BasicBSONObject ) tmpCursor.getNext();
            }
            tmpCursor.close();
            Assert.assertEquals( actRecs, obj,
                    "check datas are unequal\n" + "actDatas: " + actRecs + "\n"
                            + "expectDatas: " + minkey.toString() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() + e.getStackTrace() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.disconnect();
            System.out.println( this.getClass().getName() + " end at "
                    + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss:S" )
                            .format( new Date() ) );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

}
