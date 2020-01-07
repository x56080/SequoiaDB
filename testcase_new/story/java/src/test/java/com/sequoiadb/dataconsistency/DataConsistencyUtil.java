package com.sequoiadb.dataconsistency;

import java.util.ArrayList;

import java.util.List;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description DataConsistencyUtil.java
 * @author wuyan
 * @date 2018.12.28
 */

public class DataConsistencyUtil extends SdbTestBase {

    // the number of inserts must be an integer of 1w
    public static ArrayList< BSONObject > insertDatas( DBCollection dbcl,
            int insertNums ) {
        return insertDatas( dbcl, insertNums, 0, 10000 );
    }

    // the number of inserts must be an integer of 1w
    public static ArrayList< BSONObject > insertDatas( DBCollection dbcl,
            int insertNums, int beginNo ) {
        return insertDatas( dbcl, insertNums, beginNo, 10000 );
    }

    public static ArrayList< BSONObject > insertDatas( DBCollection dbcl,
            int insertNums, int beginNo, int batchNums ) {
        ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
        int times = insertNums / batchNums;
        for ( int k = 0; k < times; k++ ) {
            ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
            for ( int i = 0; i < batchNums; i++ ) {
                int count = beginNo++;
                BSONObject obj = new BasicBSONObject();
                obj.put( "testa", "test" + count );
                String str = "32345.06789123456" + count;
                BSONDecimal decimal = new BSONDecimal( str );
                obj.put( "decimala", decimal );
                obj.put( "no", count );
                obj.put( "order", count );
                obj.put( "inta", count );
                obj.put( "ftest", count + 0.2345 );
                obj.put( "str", "test_" + String.valueOf( count ) );
                insertRecord.add( obj );
                insertRecords.add( obj );
            }
            dbcl.bulkInsert( insertRecord, 0 );
        }
        return insertRecords;
    }

    public static void checkDataContent( DBCollection dbcl,
            ArrayList< BSONObject > expRecord ) {
        checkDataContent( dbcl, expRecord, "" );
    }

    public static void checkDataContent( DBCollection dbcl,
            ArrayList< BSONObject > expRecord, String matcher ) {
        DBCursor cursor = dbcl.query( matcher, "", "{'order':1}", "" );
        ArrayList< BSONObject > queryRecords = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            queryRecords.add( record );
        }
        long count = dbcl.getCount();
        Assert.assertEquals( count, expRecord.size() );
        Assert.assertEquals( queryRecords, expRecord );
    }

    public static void checkDataContent( DBCollection dbcl,
            List< BSONObject > expRecord, String matcher ) {
        DBCursor cursor = dbcl.query( matcher, "", "{'order':1}", "" );
        List< BSONObject > queryRecords = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            queryRecords.add( record );
        }
        long count = dbcl.getCount();
        Assert.assertEquals( count, expRecord.size() );
        Assert.assertEquals( queryRecords, expRecord );
    }

    public static void checkDataConsistency( Sequoiadb sdb, String groupName,
            String csName, String clName, ArrayList< BSONObject > expRecord ) {
        checkDataConsistency( sdb, groupName, csName, clName, expRecord, "" );
    }

    public static void checkDataConsistency( Sequoiadb sdb, String groupName,
            String csName, String clName, ArrayList< BSONObject > expRecord,
            String matcher ) {
        List< String > nodeInfo = CommLib.getNodeAddress( sdb, groupName );
        for ( int i = 0; i < nodeInfo.size(); i++ ) {
            Sequoiadb dataDB = null;
            try {
                dataDB = new Sequoiadb( nodeInfo.get( i ), "", "" );
                DBCollection dbcl = dataDB.getCollectionSpace( csName )
                        .getCollection( clName );

                // incorrect number of records,get count again
                int eachSleepTime = 1000;
                int maxSleetTime = 600000;
                int alreadySleepTime = 0;
                long count = 0;
                do {
                    count = dbcl.getCount( matcher );
                    try {
                        Thread.sleep( eachSleepTime );
                    } catch ( InterruptedException e ) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    alreadySleepTime += eachSleepTime;
                    if ( alreadySleepTime > maxSleetTime )
                        Assert.fail(
                                "count consistency fail exceeds maximum waiting time:"
                                        + alreadySleepTime + "\nnodeinfo:"
                                        + nodeInfo.get( i ) + "\n get count:"
                                        + count );
                } while ( count != expRecord.size() );

                DBCursor cursor = dbcl.query( matcher, "", "{'order':1}", "" );
                ArrayList< BSONObject > queryRecords = new ArrayList< BSONObject >();
                while ( cursor.hasNext() ) {
                    BSONObject record = cursor.getNext();
                    queryRecords.add( record );
                }

                Assert.assertEquals( queryRecords, expRecord,
                        "\nnodeinfo:" + nodeInfo.get( i ) );
            } finally {
                dataDB.disconnect();
            }
        }
    }

    public static void checkDataConsistency( Sequoiadb sdb, String groupName,
            String csName, String clName, List< BSONObject > expRecord,
            String matcher ) {
        List< String > nodeInfo = CommLib.getNodeAddress( sdb, groupName );
        for ( int i = 0; i < nodeInfo.size(); i++ ) {
            Sequoiadb dataDB = null;
            try {
                dataDB = new Sequoiadb( nodeInfo.get( i ), "", "" );
                DBCollection dbcl = dataDB.getCollectionSpace( csName )
                        .getCollection( clName );

                // incorrect number of records,get count again
                int eachSleepTime = 1000;
                int maxSleetTime = 600000;
                int alreadySleepTime = 0;
                long count = 0;
                do {
                    count = dbcl.getCount( matcher );
                    try {
                        Thread.sleep( eachSleepTime );
                    } catch ( InterruptedException e ) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    alreadySleepTime += eachSleepTime;
                    if ( alreadySleepTime > maxSleetTime )
                        Assert.fail(
                                "count consistency fail exceeds maximum waiting time:"
                                        + alreadySleepTime + "\nnodeinfo:"
                                        + nodeInfo.get( i ) + "\n get count:"
                                        + count );
                } while ( count != expRecord.size() );

                DBCursor cursor = dbcl.query( matcher, "", "{'order':1}", "" );
                ArrayList< BSONObject > queryRecords = new ArrayList< BSONObject >();
                while ( cursor.hasNext() ) {
                    BSONObject record = cursor.getNext();
                    queryRecords.add( record );
                }
                Assert.assertEquals( queryRecords, expRecord,
                        "\nnodeinfo:" + nodeInfo.get( i ) );
            } finally {
                dataDB.disconnect();
            }

        }
    }

    public static String getGroupName( Sequoiadb sdb ) {
        ArrayList< String > rgNames = CommLib.getDataGroupNames( sdb );
        int serino = ( int ) ( Math.random() * rgNames.size() );
        String groupName = rgNames.get( serino );
        return groupName;
    }

    public static DBCollection createCL( CollectionSpace cs, String clName,
            String option ) {
        DBCollection cl = null;
        BSONObject options = ( BSONObject ) JSON.parse( option );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }

        cl = cs.createCollection( clName, options );
        return cl;
    }

    public static DBCollection createCL( CollectionSpace cs, String clName ) {
        return createCL( cs, clName, null );
    }

}
