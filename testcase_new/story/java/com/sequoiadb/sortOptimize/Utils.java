package com.sequoiadb.sortOptimize;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Utils {

    public static boolean checkSortResult( DBCollection cl, BSONObject sortObj,
            String threadName ) throws BaseException {
        System.out.println( "--------" + threadName
                + " begin to check sort result---------" );
        DBCursor queryCursor = null;

        try {
            queryCursor = cl.query( null, null, sortObj, null );
            while ( queryCursor.hasNext() ) {
                BSONObject expectObj = ( BSONObject ) queryCursor.getNext(); // the
                                                                             // pre
                                                                             // one
                if ( queryCursor.hasNext() ) {
                    BSONObject actObj = ( BSONObject ) queryCursor.getNext(); // the
                                                                              // next
                                                                              // one
                    for ( String key : sortObj.keySet() ) {
                        String expectValue = ( String ) expectObj.get( key );
                        String actValue = ( String ) actObj.get( key );
                        if ( expectValue.compareTo( actValue ) > 0 ) { // sort
                                                                       // result
                                                                       // not
                                                                       // expected
                            System.out.println( "nextResult: "
                                    + actObj.toString() + ", preResult: "
                                    + expectObj.toString() );
                            return false;
                        } else if ( expectValue.compareTo( actValue ) == 0 ) { // compare
                                                                               // next
                                                                               // key
                            continue;
                        } else {
                            break;
                        }
                    }
                }
            }
            return true;
        } catch ( BaseException e ) {
            e.printStackTrace();
            return false;
        } finally {
            System.out.println( "--------" + threadName
                    + " end to check sort result---------" );
            queryCursor.close();
        }
    }

    public static String getRandomString( int length ) {
        String str = "zxcvbnmlkjhgfdsaqwertyuiopQWERTYUIOPASDFGHJKLZXCVBNM1234567890$%!@";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length; ++i ) {
            int number = random.nextInt( 66 );
            sb.append( str.charAt( number ) );
        }
        return sb.toString();
    }

}
