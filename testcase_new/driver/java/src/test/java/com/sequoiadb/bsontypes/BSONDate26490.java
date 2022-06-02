package com.sequoiadb.bsontypes;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import static org.testng.Assert.assertEquals;

/**
 * @description seqDB-26490: java驱动使用 java.util.Date 方式读写日期数据
 * @author gongmeiyan
 * @date 2022/05/11
 * @version 1.00
 */
public class BSONDate26490 extends SdbTestBase {
    private String clName = "cl_26490";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
            sdb.createCollectionSpace( SdbTestBase.csName );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
    }

    @DataProvider(name = "generateDataProvider")
    public Object[][] generateDataProvider() {
        return new Object[][] {
                // Date with time zone problem
                { 1, "0001-01-01", "0000-12-30T00:05:43" },
                { 2, "1581-12-31", "1582-01-10T00:05:43" },
                { 3, "1899-12-31", "1899-12-31T00:05:43" },
                // Date without zone problem
                { 4, "1900-01-01", "1900-01-01T00:05:43" },
                { 5, "1900-01-02", "1900-01-02T00:00" },
                { 6, "1970-01-01", "1970-01-01T00:00" },
                { 7, "2022-04-01", "2022-04-01T00:00" },
                { 8, "9999-12-31", "9999-12-31T00:00" } };
    }

    @Test(dataProvider = "generateDataProvider")
    public void testDate( int id, String date, String expectDate )
            throws ParseException {
        // write data
        BSONObject record = new BasicBSONObject();
        Date insertDate = new SimpleDateFormat( "yyyy-MM-dd" ).parse( date );
        record.put( "id", id );
        record.put( "date", insertDate );
        cl.insertRecord( record );

        // read and check data
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "id", id );
        try ( DBCursor cursor = cl.query( matcher, null, null, null ) ;) {
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                Date actualDate = ( Date ) obj.get( "date" );

                assertEquals( actualDate.toString(), expectDate.toString(),
                        "check data are unequal\n" + "actualDate: " + actualDate
                                + "\n" + "expectDate: " + expectDate );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }
}
