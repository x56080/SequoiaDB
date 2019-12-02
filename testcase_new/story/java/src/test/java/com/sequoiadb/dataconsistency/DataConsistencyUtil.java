package com.sequoiadb.dataconsistency;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.testcommon.CommLib;

/**
 * @Description DataConsistencyUtil.java
 * @author wuyan
 * @date 2018.12.28
 */

public class DataConsistencyUtil {
    public static ArrayList< BSONObject > insertDatas( DBCollection dbcl,
            int insertNums, int beginNo ) {
        int batchNums = 10000;
        int times = insertNums / batchNums;
        int remainder = insertNums % batchNums;
        if ( remainder != 0 ) {
            times += 1;
        }

        ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >(
                insertNums );
        for ( int k = 0; k < times; k++ ) {
            if ( k == times - 1 && remainder != 0 ) {
                batchNums = remainder;
            }

            ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >(
                    batchNums );
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
            dbcl.insert( insertRecord );
        }
        return insertRecords;
    }

    public static void checkDataContent( DBCollection dbcl,
            List< BSONObject > expRecord, String matcher ) {
        DBCursor cursor = dbcl.query( matcher, "", "{'order':1}", "" );

        int count = 0;
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            Assert.assertEquals( record, expRecord.get( count++ ) );
        }
        cursor.close();
        Assert.assertEquals( count, expRecord.size() );
    }

    /**
     * 检查CL主备节点集合CompleteLSN一致
     * 
     * @param cl
     * @return boolean 如果主节点CompleteLSN小于等于备节点CompleteLSN返回true,否则返回false
     * @throws Exception
     * @author luweikang
     */
    public static boolean isCLConsistency( DBCollection cl ) throws Exception {

        Sequoiadb db = cl.getSequoiadb();
        boolean isConsistency = false;

        List< String > groupNames = FullTextDBUtils.getCLGroups( cl );
        for ( String groupName : groupNames ) {
            List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
            ReplicaGroup rg = db.getReplicaGroup( groupName );

            try ( Sequoiadb masterNode = rg.getMaster().connect()) {
                // 初始值为-2，是为了与获取到的实际lsn进行比较时，不可能相等
                long completeLSN = -2;
                DBCursor cursor = masterNode.getSnapshot(
                        Sequoiadb.SDB_SNAP_SYSTEM, null, "{CompleteLSN: ''}",
                        null );
                if ( cursor.hasNext() ) {
                    BasicBSONObject snapshot = ( BasicBSONObject ) cursor
                            .getNext();
                    if ( snapshot.containsField( "CompleteLSN" ) ) {
                        completeLSN = ( long ) snapshot.get( "CompleteLSN" );
                    }
                } else {
                    throw new Exception( masterNode.getNodeName()
                            + " can't not find system snapshot" );
                }
                cursor.close();

                for ( String nodeName : nodeNames ) {
                    if ( masterNode.getNodeName().equals( nodeName ) ) {
                        continue;
                    }
                    isConsistency = false;
                    try ( Sequoiadb nodeConn = rg.getNode( nodeName )
                            .connect()) {
                        DBCursor cur = null;
                        // 初始值为-3，是为了与获取到的实际lsn进行比较时，不可能相等
                        long checkCompleteLSN = -3;
                        // 循环比较1800s
                        for ( int i = 0; i < 1800; i++ ) {
                            cur = nodeConn.getSnapshot(
                                    Sequoiadb.SDB_SNAP_SYSTEM, null,
                                    "{CompleteLSN: ''}", null );
                            if ( cur.hasNext() ) {
                                BasicBSONObject checkSnapshot = ( BasicBSONObject ) cur
                                        .getNext();
                                if ( checkSnapshot
                                        .containsField( "CompleteLSN" ) ) {
                                    checkCompleteLSN = ( long ) checkSnapshot
                                            .get( "CompleteLSN" );
                                }
                            }
                            cur.close();

                            if ( completeLSN <= checkCompleteLSN ) {
                                isConsistency = true;
                                break;
                            }
                            try {
                                Thread.sleep( 1000 );
                            } catch ( InterruptedException e ) {
                                e.printStackTrace();
                            }
                        }
                        if ( !isConsistency ) {
                            throw new Exception( "Group [" + groupName
                                    + "] node system snapshot is not the same, masterNode "
                                    + masterNode.getNodeName()
                                    + " CompleteLSN: " + completeLSN + ", "
                                    + nodeName + " CompleteLSN: "
                                    + checkCompleteLSN );
                        }
                    }
                }
            }
        }

        return isConsistency;
    }

    public static void checkDataConsistency( Sequoiadb sdb, String csName,
            String clName, List< BSONObject > expRecord, String matcher )
            throws Exception {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        Assert.assertTrue( isCLConsistency( cl ) );
        checkDataContent( cl, expRecord, matcher );
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

    public static boolean isOneNodeInGroup( Sequoiadb db, String groupName ) {
        ReplicaGroup rg = db.getReplicaGroup( groupName );
        BSONObject detail = rg.getDetail();
        BasicBSONList group = ( BasicBSONList ) detail.get( "Group" );
        if ( group.size() <= 1 ) {
            return true;
        }
        return false;
    }

    public static void createUnquieIndexes( CollectionSpace cs,
            String clName ) {
        DBCollection dbcl = cs.getCollection( clName );
        dbcl.createIndex( "testa", "{no:1}", true, false );
        dbcl.createIndex( "testb", "{inta:1,no:1}", true, false );
        dbcl.createIndex( "testc", "{str:1,no:1}", true, false );
        dbcl.createIndex( "teste", "{ftest:1,no:-1}", true, false );
        dbcl.createIndex( "testf", "{ftest:-1,no:1}", true, false );
        dbcl.createIndex( "testg", "{str:-1,order:1,no:-1}", true, false );
    }
}
