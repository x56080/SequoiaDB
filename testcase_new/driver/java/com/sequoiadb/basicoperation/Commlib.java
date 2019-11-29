package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.regex.Pattern;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.testng.Assert;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.*;

/**
 * FileName: Commlib.java public call function for test basicOperation
 * 
 * @author wuyan
 * @Date 2016.9.19
 * @version 1.00
 */
public class Commlib {
    public static ArrayList< String > groupList;

    /**
     * insert Datas
     * 
     * @param 1.cl:eg:db.cs.cl
     *            2.records:insert records
     */
    public static void insertDatas( DBCollection cl, String[] records ) {
        // insert records
        for ( int i = 0; i < records.length; i++ ) {
            try {
                cl.insert( records[ i ] );
            } catch ( BaseException e ) {
                Assert.assertTrue( false,
                        "insert jsonDatas fail " + e.getMessage() );
            }
        }
        long count = cl.getCount();
        Assert.assertEquals( records.length, count,
                "insert datas count error" );
    }

    public static boolean isStandAlone( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() == -159 ) {
                System.out.printf( "run mode is standalone" );
                return true;
            }
        }
        return false;
    }

    public static ArrayList< String > getDataGroups( Sequoiadb sdb ) {
        // ArrayList<String> groupList = new ArrayList<String>();
        try {
            groupList = sdb.getReplicaGroupNames();
            groupList.remove( "SYSCatalogGroup" );
            groupList.remove( "SYSCoord" );
            groupList.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "getDataGroups fail " + e.getMessage() );
        }
        return groupList;
    }

    public static boolean OneGroupMode( Sequoiadb sdb ) {
        if ( getDataGroups( sdb ).size() < 2 ) {
            System.out.printf( "only one group" );
            return true;
        }
        return false;
    }

    public static String getSourceRGName( Sequoiadb sdb, String csName,
            String clName ) {
        String groupName = "";
        String cond = String.format( "{Name:\"%s.%s\"}", csName, clName );
        DBCursor collections = sdb.getSnapshot( 8, cond, null, null );
        while ( collections.hasNext() ) {
            BSONObject collection = collections.getNext();
            BasicBSONObject doc = ( BasicBSONObject ) collection;
            doc.getString( "Name" );
            BasicBSONList subdoc = ( BasicBSONList ) doc.get( "CataInfo" );
            BasicBSONObject elem = ( BasicBSONObject ) subdoc.get( 0 );
            groupName = elem.getString( "GroupName" );
        }
        return groupName;
    }

    public static String getTarRgName( Sequoiadb sdb, String sourceRGName ) {
        String tarRgName = "";
        for ( int i = 0; i < groupList.size(); ++i ) {
            String name = groupList.get( i );
            if ( !name.equals( sourceRGName ) ) {
                tarRgName = name;
                break;
            }
        }
        return tarRgName;
    }

    public static void bulkInsert( DBCollection cl ) {
        try {
            List< BSONObject > list = new ArrayList< BSONObject >();
            long num = 10;
            for ( long i = 0; i < num; i++ ) {
                BSONObject obj = new BasicBSONObject();
                ObjectId id = new ObjectId();
                obj.put( "_id", id );
                obj.put( "test", "test" + i );
                // insert the decimal type data
                String str = "32345.067891234567890123456789" + i;
                BSONDecimal decimal = new BSONDecimal( str );
                obj.put( "decimal", decimal );
                obj.put( "no", i );
                obj.put( "str", "test_" + String.valueOf( i ) );
                // the numberlong type data
                BSONObject numberlong = new BasicBSONObject();
                numberlong.put( "$numberLong", "-9223372036854775808" );
                obj.put( "numlong", numberlong );
                // the obj type
                BSONObject subObj = new BasicBSONObject();
                subObj.put( "a", 1 + i );
                obj.put( "obj", subObj );
                // the array type
                BSONObject arr = new BasicBSONList();
                arr.put( "0", ( int ) ( Math.random() * 100 ) );
                arr.put( "1", "test" );
                arr.put( "2", 2.34 );
                obj.put( "arr", arr );
                obj.put( "boolf", false );
                // the data type
                Date now = new Date();
                obj.put( "date", now );
                // the regex type
                Pattern regex = Pattern.compile( "^2001",
                        Pattern.CASE_INSENSITIVE );
                obj.put( "binary", regex );
                list.add( obj );
            }
            cl.bulkInsert( list, DBCollection.FLG_INSERT_CONTONDUP );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "bulkinsert fail " + e.getErrorCode() + e.getMessage() );
        }
    }

}
