package com.sequoiadb.schema;

import java.util.*;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.index.IndexUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.DBCollection;
import org.bson.util.JSON;
import org.testng.Assert;

public class SchemaUtils {
    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum, int length ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        int batchNum = 5000;
        if ( recordNum < batchNum ) {
            batchNum = recordNum;
        }
        int count = 0;
        for ( int i = 0; i < recordNum / batchNum; i++ ) {
            List< BSONObject > batchRecords = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNum; j++ ) {
                String stringValue = getRandomString( length );
                int value = count++;
                BSONObject obj = new BasicBSONObject();
                obj.put( "a", value );
                obj.put( "b", stringValue );
                obj.put( "c", value );
                obj.put( "d", value );
                batchRecords.add( obj );
            }
            dbcl.bulkInsert( batchRecords );
            insertRecord.addAll( batchRecords );
            batchRecords.clear();
        }
        return insertRecord;
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum ) {
        return insertData( dbcl, recordNum, 5 );
    }

    public static String getRandomString( int length ) {
        String str = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^asssgggg!@#$";
        StringBuilder sbBuilder = new StringBuilder();

        // random generation 80-length string.
        Random random = new Random();
        StringBuilder subBuilder = new StringBuilder();
        int strLen = str.length();
        for ( int i = 0; i < strLen; i++ ) {
            int number = random.nextInt( strLen );
            subBuilder.append( str.charAt( number ) );
        }

        // generate a string at a specified length by subBuffer
        int times = length / str.length();
        for ( int i = 0; i < times; i++ ) {
            sbBuilder.append( subBuilder );
        }
        int subTimes = length % str.length();
        if ( subTimes != 0 ) {
            sbBuilder.append( str.substring( 0, subTimes ) );
        }
        return sbBuilder.toString();
    }

    public static void checkAddSchema( Sequoiadb db, String csName,
            String clName, String schemaName ) {
        String clFullName = csName + "." + clName;
        // check SDB_SNAP_CATALOG
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", clFullName );
        matcher.put( "Schema", schemaName );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG, matcher,
                null, null );
        Assert.assertTrue( cursor.hasNext(), "cl : " + clFullName
                + ", schema : " + schemaName
                + ", cl add schema failed! catalog snapshot is empty." );
        cursor.close();

        // check SDB_LIST_SCHEMAS
        BasicBSONObject query = new BasicBSONObject();
        query.put( "Collection", clFullName );
        query.put( "Name", schemaName );
        cursor = db.getList( Sequoiadb.SDB_LIST_SCHEMAS, query, null, null );
        Assert.assertTrue( cursor.hasNext(),
                "cl : " + clFullName + ", schema : " + schemaName
                        + ", cl add schema failed! schemas list is empty." );
        cursor.close();
    }

    public static void checkConsistence( Sequoiadb db, String csName,
            String clName, BasicBSONObject selector, BasicBSONObject orderBy,
            List< BSONObject > expRecords, List< BSONObject > expPrimalecords,
            Boolean exceptPrimal ) {
        List< String > clNodes = IndexUtils.getClNodes( db, csName, clName );
        DBCollection dbcl;
        DBCursor cursor;
        for ( String clNode : clNodes ) {
            try ( Sequoiadb data = new Sequoiadb( clNode, "", "" )) {
                dbcl = data.getCollectionSpace( csName )
                        .getCollection( clName );
                cursor = dbcl.query( null, selector, orderBy, null );
                checkRecords( cursor, expRecords );
                if ( exceptPrimal ) {
                    cursor = dbcl.query( null, selector, orderBy, null,
                            4194304 );
                    checkRecords( cursor, expPrimalecords );
                }
            } catch ( AssertionError e ) {
                System.out.println( "data : " + clNode + ", cl : " + csName
                        + "." + clName );
                throw e;
            }
        }
    }

    public static void checkConsistence( Sequoiadb db, String csName,
            String clName, BasicBSONObject selector, BasicBSONObject orderBy,
            List< BSONObject > expRecords,
            List< BSONObject > expPrimalecords ) {
        checkConsistence( db, csName, clName, selector, orderBy, expRecords,
                expPrimalecords, true );
    }

    public static void checkRecords( DBCursor cursor,
            List< BSONObject > expRecords ) {
        int count = 0;
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            BSONObject expRecord = expRecords.get( count++ );
            Assert.assertEquals( record, expRecord );
        }
        cursor.close();
        Assert.assertEquals( count, expRecords.size() );
    }

    public static void checkColumnDef( Sequoiadb db, String schemaName,
            BasicBSONObject expColumnDef ) {
        for ( Map.Entry< String, Object > entry : expColumnDef.entrySet() ) {
            BasicBSONObject value = ( BasicBSONObject ) entry.getValue();
            if ( value.get( "Restrict" ) == null ) {
                value.put( "Restrict", 0 );
            }
            if ( value.get( "RestrictDesc" ) == null ) {
                value.put( "RestrictDesc", "" );
            }
        }

        DBCursor cursor = db.getList( Sequoiadb.SDB_LIST_SCHEMAS,
                new BasicBSONObject( "Name", schemaName ), null, null );
        BasicBSONObject actColumnDef = ( BasicBSONObject ) cursor.getNext()
                .get( "Columns" );
        cursor.close();
        System.out.println( "actColumnDef -- " + actColumnDef.toString() );
        System.out.println( "expColumnDef -- " + expColumnDef.toString() );
        Assert.assertEquals( actColumnDef, expColumnDef );
    }

    public static void checkColumnDef( Sequoiadb db, String schemaName,
            String expColumnDef ) {
        checkColumnDef( db, schemaName,
                ( BasicBSONObject ) JSON.parse( expColumnDef ) );
    }

    public static void dropSchema( Sequoiadb db, String schemaName ) {
        try {
            db.dropSchema( schemaName );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_SCHEMA_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
    }
}
