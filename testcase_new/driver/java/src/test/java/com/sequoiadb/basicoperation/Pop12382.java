package com.sequoiadb.basicoperation;

import java.util.List;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * FileName: Pop12382.java test content:test pop interface for cappedCL
 * 
 * @author liuxiaoxuan
 * @Date 2017.8.14
 */
public class Pop12382 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection cappedCL = null;
    private String cappedCSName = "story_java_cappedCS_12382";
    private String cappedCLName = "cappedCL_12382";
    private List< BSONObject > insrtObjs = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        try {
            boolean isCapped = true;
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            cappedCL = CappedCLUtils.createCL( sdb, cappedCSName, cappedCLName,
                    isCapped );
            int recordNums = 10;
            insertRecords( recordNums );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public void insertRecords( int recordNums ) {
        for ( int i = 0; i < recordNums; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{ a :" + i + "}" );
            cappedCL.insert( obj );
        }

        BSONObject orderBy = new BasicBSONObject();
        orderBy.put( "_id", 1 );
        DBCursor cursor = cappedCL.query( null, null, orderBy, null );
        while ( cursor.hasNext() ) {
            insrtObjs.add( cursor.getNext() );
        }
        cursor.close();
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( cappedCSName );
            if ( cs != null && cs.isCollectionExist( cappedCLName ) ) {
                sdb.dropCollectionSpace( cappedCSName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void testPop() {
        boolean isHasDirection = true;
        Assert.assertEquals( popValidRecords( isHasDirection ), true,
                "pop valid records failed" );
        Assert.assertEquals( popValidRecords( !isHasDirection ), true,
                "pop valid records failed" );
        Assert.assertEquals( popInvalidLogicalID(), -6,
                "pop invalid records success" );
        Assert.assertEquals( popHasNotLogicalID(), -6,
                "pop invalid records success" );
        Assert.assertEquals( popInvalidDirection(), -6,
                "pop invalid records success" );
    }

    public boolean popValidRecords( boolean isHasDirection ) {
        BSONObject orderBy = new BasicBSONObject();
        orderBy.put( "_id", 1 );
        DBCursor cursor = cappedCL.query( null, null, orderBy, null );

        int direction = 1;
        long logicalId = 0;

        try {
            while ( cursor.hasNext() ) {
                logicalId = ( long ) cursor.getNext().get( "_id" );
                break;
            }

            // pop the first record from CL
            BSONObject popObj = new BasicBSONObject();
            popObj.put( "LogicalID", logicalId );
            if ( isHasDirection ) {
                popObj.put( "Direction", direction );
            }
            cappedCL.pop( popObj );

            // remove the first elem from expectList
            for ( Iterator< BSONObject > it = insrtObjs.iterator(); it
                    .hasNext(); ) {
                it.next();
                it.remove();
                break;
            }

            long actBsonSize = cappedCL.getCount();
            long eptBsonSize = insrtObjs.size();
            System.out.println( "actBsonSize: " + actBsonSize + " "
                    + "eptBsonSize: " + eptBsonSize );

            // check the records size
            if ( actBsonSize != eptBsonSize ) {
                return false;
            }

            // check every records
            cursor = cappedCL.query( null, null, orderBy, null );
            int i = 0;
            while ( cursor.hasNext() && i < insrtObjs.size() ) {
                BSONObject act = cursor.getNext();
                BSONObject exp = insrtObjs.get( i );
                System.out.println( "act.toString(): " + act.toString() + " "
                        + "exp.toString(): " + exp.toString() );
                if ( !act.toString().equals( exp.toString() ) ) {
                    return false;
                }
                i++;
            }

        } catch ( BaseException e ) {
            System.out.println( "pop valid records error: " + e.getErrorCode()
                    + " " + e.getMessage() );
            return false;
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
        return true;
    }

    public int popInvalidLogicalID() {
        long logicalId = 99999;
        int direction = 1;

        try {
            BSONObject popObj = new BasicBSONObject();
            popObj.put( "LogicalID", logicalId );
            popObj.put( "Direction", direction );
            cappedCL.pop( popObj );
            return 0;
        } catch ( BaseException e ) {
            System.out.println( "logicalId 99999 error: " + e.getErrorCode()
                    + " " + e.getMessage() );
            return e.getErrorCode();
        }
    }

    public int popHasNotLogicalID() {
        int direction = 1;

        try {
            BSONObject popObj = new BasicBSONObject();
            popObj.put( "Direction", direction );
            cappedCL.pop( popObj );
            return 0;
        } catch ( BaseException e ) {
            System.out.println( "logicalId none error: " + e.getErrorCode()
                    + " " + e.getMessage() );
            return e.getErrorCode();
        }
    }

    public int popInvalidDirection() {
        double direction = 1.1;
        long logicalId = 0;
        BSONObject orderBy = new BasicBSONObject();
        orderBy.put( "_id", 1 );
        DBCursor cursor = cappedCL.query( null, null, orderBy, null );

        try {
            while ( cursor.hasNext() ) {
                logicalId = ( long ) cursor.getNext().get( "_id" );
                break;
            }
            BSONObject popObj = new BasicBSONObject();
            popObj.put( "LogicalID", logicalId );
            popObj.put( "Direction", direction );
            cappedCL.pop( popObj );
            return 0;
        } catch ( BaseException e ) {
            System.out.println( "direction 1.1 error: " + e.getErrorCode() + " "
                    + e.getMessage() );
            return e.getErrorCode();
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
    }
}
