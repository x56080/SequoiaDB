package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;

/**
 * FileName: IndexUtils.java public call function for test index
 * 
 * @author wuyan
 * @Date 2019.3.27
 * @version 1.00
 */
public class IndexUtils {

    public static DBCollection createCSAndCL( Sequoiadb sdb, String csName,
            String clName, int pagesize ) {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        BSONObject options = new BasicBSONObject();
        options.put( "PageSize", pagesize );
        CollectionSpace cs = sdb.createCollectionSpace( csName, options );
        DBCollection dbcl = cs.createCollection( clName );
        return dbcl;
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum, int length ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordNum; i++ ) {
            String keyValue = getRandomString( length );
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", keyValue );
            obj.put( "no", i );
            insertRecord.add( obj );
        }
        dbcl.insert( insertRecord );
        return insertRecord;
    }

    public static void checkRecords( DBCollection dbcl,
            List< BSONObject > expRecord, String matcher, String hint ) {
        DBCursor cursor = dbcl.query( matcher, "", "{'no':1}", hint );
        int count = 0;
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            Assert.assertEquals( record, expRecord.get( count++ ) );
        }
        cursor.close();
        Assert.assertEquals( count, expRecord.size() );
    }

    public static String getRandomString( int length ) {
        String str = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^asssgggg!@#$";
        StringBuffer sbBuffer = new StringBuffer();
        // random generation 80-length string.
        Random random = new Random();
        StringBuffer subBuffer = new StringBuffer();
        int strLen = str.length();
        for ( int i = 0; i < strLen; i++ ) {
            int number = random.nextInt( strLen );
            subBuffer.append( str.charAt( number ) );
        }

        // generate a string at a specified length by subBuffer
        int times = length / str.length();
        for ( int i = 0; i < times; i++ ) {
            sbBuffer.append( subBuffer );
        }
        int subTimes = length % str.length();
        if ( subTimes != 0 ) {
            sbBuffer.append( str.substring( 0, subTimes ) );
        }
        return sbBuffer.toString();
    }

}
