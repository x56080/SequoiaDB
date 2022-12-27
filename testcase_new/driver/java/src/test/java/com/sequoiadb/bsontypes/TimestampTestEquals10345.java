package com.sequoiadb.bsontypes;

import java.util.Date;
import java.util.HashMap;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TimestampTestEquals10345.java* test interface: equals (Object obj)
 * TestLink: seqDB-10345:
 * TestLink: seqDB-29718:
 *
 * @author wuyan
 * @Date 2016.10.14
 * @version 1.00
 */
public class TimestampTestEquals10345 extends SdbTestBase {
    private static Sequoiadb sdb = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void testGetDateAndToString() {
        BSONObject obj = new BasicBSONObject();
        int seconds = 23456;
        int inc = 99988;
        String expectTime = "{ $timestamp : 1970-01-01-14.30.56.99988}";
        BSONTimestamp timestamp = new BSONTimestamp( seconds, inc );
        BSONTimestamp timestamp1 = new BSONTimestamp( seconds, inc );
        obj.put( "time", expectTime );

        // test equals
        Assert.assertEquals( timestamp.equals( timestamp ), true,
                "check timestamp self fail" );
        Assert.assertEquals( timestamp.equals( timestamp1 ), true,
                "check timestamp self fail" );
        Assert.assertEquals( timestamp.equals( obj.get( "time" ) ), false,
                "check the get object" );
        Assert.assertEquals( timestamp.equals( null ), false,
                "check null fail" );

        //test hash map
        HashMap<BSONTimestamp, String> map = new HashMap<>() ;

        Assert.assertEquals( timestamp.hashCode(), timestamp1.hashCode() );
        map.put( timestamp, "timestamp" ) ;
        map.put( timestamp1, "timestamp1") ;
        Assert.assertEquals( 1, map.size() );
        Assert.assertEquals( map.get( timestamp ), map.get( timestamp1 ) );

        map.clear();
        BSONTimestamp reference = timestamp;
        Assert.assertEquals( timestamp.hashCode(), reference.hashCode() );
        map.put( timestamp, "timestamp");
        map.put( reference, "reference" );
        Assert.assertEquals( 1, map.size() );
        Assert.assertEquals( map.get( timestamp ), map.get( reference ) );

        map.clear();
        BSONTimestamp nowTimestamp = new BSONTimestamp( new Date() ) ;
        Assert.assertNotEquals( timestamp.hashCode(), nowTimestamp.hashCode() );
        map.put( timestamp, "timestamp" ) ;
        map.put( nowTimestamp, "nowTimestamp") ;
        Assert.assertEquals( 2, map.size() );
        Assert.assertNotEquals( map.get( nowTimestamp ), map.get( timestamp ) );
    }

    @AfterClass
    public void tearDown() {
        sdb.close();
    }

}
