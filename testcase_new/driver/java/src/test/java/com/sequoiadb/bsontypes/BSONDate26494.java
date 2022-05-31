package com.sequoiadb.bsontypes;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.TimeZone;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDate;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description seqDB-26493 :: java驱动以java.util.Date方式写日期数据，使用LocalDate方式读日期数据
 * @author wuyan
 * @date 2021.5.13
 * @version 1.10
 */
public class BSONDate26494 extends SdbTestBase {
    private String clName = "baseDate_26494";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;
    private ArrayList< BSONObject > queryRecs = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
    }

    @DataProvider(name = "generateDataProvider")
    public Object[][] generateDataProvider() {
        return new Object[][] {
                // 1900-01-01之前存在时区问题的数据
                { 1, "0001-01-01", "0000-12-30T00:05:43", "yyyy-MM-dd" },
                { 2, "1581-12-31", "1582-01-10T00:05:43", "yyyy-MM-dd" },
                { 3, "1582-01-31", "1582-02-10T00:05:43", "yyyy-MM-dd" },
                { 4, "1899-12-31 23:59:59", "1900-01-01T00:05:42",
                        "yyyy-MM-dd HH:mm:ss" },
                // 1900-01-01之后不存在时区问题的数据
                { 5, "1900-01-01 23:00:01", "1900-01-01T23:00:01",
                        "yyyy-MM-dd HH:mm:ss" },
                { 6, "1900-01-02", "1900-01-02T00:00", "yyyy-MM-dd" },
                { 7, "1970-01-01 12:59:59", "1970-01-01T12:59:59",
                        "yyyy-MM-dd HH:mm:ss" },
                { 8, "2021-01-31", "2021-01-31T00:00", "yyyy-MM-dd" },
                { 9, "2022-12-31 01:14:23", "2022-12-31T00:00", "yyyy-MM-dd" },
                { 10, "2037-12-31", "2037-12-31T00:00", "yyyy-MM-dd" },
                { 11, "9999-12-31", "9999-12-31T00:00", "yyyy-MM-dd" } };
    }

    @Test(dataProvider = "generateDataProvider")
    public void test( int no, String insertDate, String expDate,
            String dfParttern ) throws ParseException {
        SimpleDateFormat df = new SimpleDateFormat( dfParttern );
        Date date = df.parse( insertDate );
        BasicBSONObject obj = new BasicBSONObject();
        obj.put( "no", no );
        obj.put( "date", date );
        cl.insertRecord( obj );

        // check the insert result
        BSONObject tmp = new BasicBSONObject();
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "no", no );
        DBCursor cursor = cl.query( matcher, null, null, null );
        String actQueryDate = "";
        while ( cursor.hasNext() ) {
            BSONObject queryObj = ( BSONObject ) cursor.getNext();
            queryRecs.add( queryObj );
            BSONDate bsonDate = ( BSONDate ) queryObj.get( "date" );
            LocalDateTime time = bsonDate.toLocalDateTime();
            actQueryDate = time.toString();
        }
        cursor.close();
        Assert.assertEquals( actQueryDate, expDate,
                "---query reces = " + queryRecs );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }

}
