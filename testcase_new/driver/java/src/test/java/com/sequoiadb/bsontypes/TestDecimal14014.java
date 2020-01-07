package com.sequoiadb.bsontypes;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.ObjectId;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.Date;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertNull;

/**
 * Created by laojingtang on 18-1-15.
 */
public class TestDecimal14014 extends SdbTestBase {
    private static String CLNAME = TestDecimal14014.class.getSimpleName();
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection dbcl;
    private SimpleDateFormat sdf = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss.SSS" );

    @BeforeClass
    public void setUp() {
        System.out.println( this.getClass().getName() + " begin at "
                + sdf.format( new Date() ) );
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        dbcl = cs.createCollection( CLNAME );
    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( CLNAME );
        sdb.disconnect();
        System.out.println( this.getClass().getName() + " end at "
                + sdf.format( new Date() ) );
    }

    /**
     * 1、插入特殊decimal值并查询 2、删除特殊decimal值后查询
     */
    @Test
    public void test() {
        String[] specialValue = { "MAX", "MIN", "NAN", ".1", "-1", "+1" };
        for ( String s : specialValue ) {
            BSONObject expect = new BasicBSONObject( "a", new BSONDecimal( s ) )
                    .append( "_id", new ObjectId() );
            // insert
            dbcl.insert( expect );
            BSONObject actual = dbcl.queryOne();
            assertEquals( actual, expect );
            // del
            dbcl.delete( new BasicBSONObject( "_id", expect.get( "_id" ) ) );
            assertNull( dbcl.queryOne() );
        }
    }
}
