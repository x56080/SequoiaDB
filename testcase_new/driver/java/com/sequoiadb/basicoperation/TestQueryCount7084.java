package com.sequoiadb.basicoperation;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestQuery7084 query(); query (DBQuery matcher); getCount ();
 *                         getCount (String matcher); getCount (BSONObject
 *                         condition, BSONObject hint)
 * @author chensiqin * @Date 2016-09-19
 * @version 1.00
 */

public class TestQueryCount7084 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7084";
    private ArrayList< BSONObject > insertRecods;

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        try {
            System.out.println( "the TestCase Name:" + this.getClass().getName()
                    + ". the TestCase begin at:"
                    + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss.SSS" )
                            .format( new Date() ) );
            this.sdb = new Sequoiadb( coordAddr, "", "" );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            createCL();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQueryCount7084 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    public void createCL() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.cl = this.cs.createCollection( clName );
        this.cl.createIndex( "ageIndex", ( BSONObject ) JSON.parse( "{age:1}" ),
                false, false );
    }

    @Test
    public void test() {
        try {
            insertData();
            checkQuery();
            DBQuery dbQuery = new DBQuery();
            dbQuery.setFlag( DBQuery.FLG_QUERY_WITH_RETURNDATA );
            dbQuery.setHint(
                    ( BSONObject ) JSON.parse( "{\"\":\"ageIndex\"}" ) );
            dbQuery.setMatcher( ( BSONObject ) JSON.parse( "{age:{$gt:5}}" ) );
            dbQuery.setOrderBy( ( BSONObject ) JSON.parse( "{age:1}" ) );
            dbQuery.setReturnRowsCount( ( long ) 5 );
            dbQuery.setSelector( ( BSONObject ) JSON.parse(
                    "{name:{$default:\"zhangsan\"},age:{$default:0},num:{$default:0}}" ) );
            dbQuery.setSkipRowsCount( ( long ) 2 );
            checkQuery( dbQuery );
            checkCount();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQueryCount7084 test error, error description:"
                            + e.getMessage() );
        }
    }

    public void insertData() {
        try {
            BSONObject bson;
            this.insertRecods = new ArrayList< BSONObject >();
            for ( int i = 0; i < 10; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "name", "zhangsan" + i );
                bson.put( "age", i );
                bson.put( "num", i );
                this.insertRecods.add( bson );
            }
            this.cl.bulkInsert( this.insertRecods, 0 );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQueryCount7084 insertData error, error description:"
                            + e.getMessage() );
        }
    }

    public void checkQuery() {
        try {
            DBCursor cursor = this.cl.query();
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            if ( cursor != null ) {
                cursor.close();
            }
            Assert.assertEquals( actualList, this.insertRecods );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQueryCount7084 checkQuery error :"
                            + e.getMessage() );
        }
    }

    public void checkQuery( DBQuery dbQuery ) {
        try {
            DBCursor cursor = this.cl.query( dbQuery );
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            cursor.close();
            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            for ( int i = 8; i < 10; i++ ) {
                BSONObject obj = insertRecods.get( i );
                obj.removeField( "_id" );
                expectedList.add( obj );
            }
            Assert.assertEquals( actualList, expectedList );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQueryCount7084 checkQuery(DBQuery dbQuery) error :"
                            + e.getMessage() );
        }
    }

    public void checkCount() {
        try {
            long count;
            count = cl.getCount();
            Assert.assertEquals( count, 10 );

            String countConditionString = "{age:{$gt:5},name:\"zhangsan8\"}";
            count = cl.getCount( countConditionString );
            Assert.assertEquals( count, 1 );

            BSONObject bsonObjectCondition = ( BSONObject ) JSON
                    .parse( "{age:{$lt:5}}" );
            BSONObject bsonObjectHint = ( BSONObject ) JSON
                    .parse( "{\"\":\"ageIndex\"}" );
            count = cl.getCount( bsonObjectCondition, bsonObjectHint );
            Assert.assertEquals( count, 5 );
        } catch ( BaseException e ) {
            Assert.fail( "Sequoiadb driver TestQueryCount7084 checkCount error:"
                    + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            System.out.println( "the TestCase Name:" + this.getClass().getName()
                    + ". the TestCase end at:"
                    + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss.SSS" )
                            .format( new Date() ) );
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            this.sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "Sequoiadb driver TestQueryCount7084 tearDown error:"
                    + e.getMessage() );
        }
    }
}
