package com.sequoiadb.faulttolerance;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;

public class FaultToleranceUtils {
    public static void changeNodeStatus( String csName, String clName,
            int errorCode ) {
        Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            StringBuffer strBuffer = new StringBuffer();
            for ( int len = 0; len < 10000; len++ ) {
                strBuffer.append( "aaaaaaaaaa" );
            }

            for ( int i = 0; i < 1000; i++ ) {
                List< BSONObject > insertor = new ArrayList<>();
                for ( int j = 0; j < 1000; j++ ) {
                    insertor.add( ( BSONObject ) JSON.parse(
                            "{ 'a': '" + strBuffer.toString() + "' }" ) );
                }
                sdb.getCollectionSpace( csName ).getCollection( clName )
                        .insert( insertor, 0 );
            }
            Assert.fail( "node status is not be changed!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -252 && e.getErrorCode() != errorCode ) {
                throw e;
            }
        } finally {
            sdb.close();
        }
    }

    public static String getNodeFTStatus( Sequoiadb sdb, NodeWrapper nodes ) {
        DBCursor cursor = sdb.getSnapshot(
                Sequoiadb.SDB_SNAP_DATABASE, "{ 'RawData': true, 'NodeName': '"
                        + nodes.connect().toString() + "' }",
                "{ 'FTStatus': '' }", null );
        String ftStatus = cursor.getNext().get( "FTStatus" ).toString();
        cursor.close();
        return ftStatus;
    }

    public static String getNodeFTStatus( Sequoiadb db, String nodeName ) {
        String FTStatus = null;
        DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                "{'NodeName': '" + nodeName + "', 'RawData': true}",
                "{'FTStatus': ''}", null );
        while ( cur.hasNext() ) {
            FTStatus = ( String ) cur.getNext().get( "FTStatus" );
        }
        cur.close();
        return FTStatus;
    }

    public static void checkNodeStatus( String nodeName, String ftStatus ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            int eachSleepTime = 2;
            int maxWaitTime = 600000;
            int alreadyWaitTime = 0;
            String actFtStatus;
            do {
                DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                        "{ 'RawData': true, 'NodeName': '" + nodeName + "' }",
                        "{ 'FTStatus': '' }", null );
                actFtStatus = cursor.getNext().get( "FTStatus" ).toString();
                cursor.close();

                try {
                    Thread.sleep( eachSleepTime );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
                alreadyWaitTime += eachSleepTime;
                if ( alreadyWaitTime > maxWaitTime ) {
                    Assert.fail( "---node status is " + ftStatus
                            + " in maxWaitTime ! waitTime is"
                            + alreadyWaitTime );
                }
            } while ( !actFtStatus.equals( ftStatus ) );
        }

    }

    public static void insertError( String csName, String clName,
            int errorCode ) {
        Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            sdb.getCollectionSpace( csName ).getCollection( clName )
                    .insert( "{ 'a': 0 } " );
            if ( errorCode != 0 ) {
                Assert.fail(
                        "insert record to " + clName + " should be failed!" );
            }
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != errorCode ) {
                throw e;
            }
        } finally {
            sdb.close();
        }
    }

}
