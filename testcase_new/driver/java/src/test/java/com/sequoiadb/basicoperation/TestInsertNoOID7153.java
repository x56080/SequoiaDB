package com.sequoiadb.basicoperation;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;

import org.testng.Assert;
import org.testng.AssertJUnit;
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
 * FileName: TestInsertNoOID7153.java test interface: ensureOID (boolean flag)
 * isOIDEnsured ()
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestInsertNoOID7153 extends SdbTestBase {
    private String clName = "cl_7153";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        System.out.println( "---" + this.getClass().getName() + " begin at "
                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss:S" )
                        .format( new Date() ) );
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            System.out.printf( "connect %s failed, errMsg:%s\n",
                    SdbTestBase.coordUrl, e.getMessage() );
            AssertJUnit.assertTrue( false );
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

    /**
     * test isOIDEnsured() ,and ensureOID(boolean flag), set flag=true
     */
    public void bulkInsert() {
        try {
            List< BSONObject > list = new ArrayList< BSONObject >();
            BSONObject obj = new BasicBSONObject();
            obj.put( "no", 1 );
            obj.put( "str", "test_" + String.valueOf( 100 ) );
            list.add( obj );
            cl.ensureOID( true );
            cl.bulkInsert( list, DBCollection.FLG_INSERT_CONTONDUP );

            // check if there is a _id, if _id not exist then error
            Assert.assertEquals( true, list.toString().contains( "_id" ),
                    "the _id is not exist" );

            // check the interface:isOIDEnsured(),the return is true
            Assert.assertEquals( true, cl.isOIDEnsured(),
                    "the isOIDEnsured is error" + cl.isOIDEnsured() );

            // check the _id of client generation insert success.
            BSONObject listObj = ( BasicBSONObject ) list.get( 0 );
            Object idValue = listObj.get( "_id" );
            DBCursor tmpCursor = cl.query();
            BasicBSONObject actRecs = null;
            while ( tmpCursor.hasNext() ) {
                actRecs = ( BasicBSONObject ) tmpCursor.getNext();
            }
            Assert.assertEquals( idValue, actRecs.get( "_id" ),
                    "actIdValue: " + actRecs.get( "_id" ) );
            System.out.println( "---bulkinsert Datas is ok" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "bulkinsert fail " + e.getErrorType() );
        }

    }

    /**
     * test isOIDEnsured() ,and ensureOID(boolean flag), set flag=false
     */
    public void bulkInsertNoId() {
        try {
            List< BSONObject > list = new ArrayList< BSONObject >();
            BSONObject obj = new BasicBSONObject();
            obj.put( "str", "test_" + String.valueOf( 100 ) );
            list.add( obj );
            cl.ensureOID( false );
            cl.bulkInsert( list, 0 );
            // check if there is a _id, if _id not exist then error
            Assert.assertEquals( false, list.toString().contains( "_id" ),
                    "the _id is exist" );

            // check the interface:isOIDEnsured(),the return is true
            Assert.assertEquals( false, cl.isOIDEnsured(),
                    "the isOIDEnsured is error" + cl.isOIDEnsured() );

            // check insert result.the sdb exist id
            DBCursor tmpCursor = cl.query();
            BasicBSONObject actRecs = null;
            while ( tmpCursor.hasNext() ) {
                actRecs = ( BasicBSONObject ) tmpCursor.getNext();
            }
            Assert.assertEquals( true, actRecs.toString().contains( "_id" ) );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "bulkInsertNoId fail " + e.getErrorType() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.disconnect();
            System.out.println( "---" + this.getClass().getName() + " end at "
                    + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss:S" )
                            .format( new Date() ) );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

    @Test
    public void testInsertToBson() {
        try {
            bulkInsert();
            bulkInsertNoId();
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false, e.getMessage() );
        }
    }
}
