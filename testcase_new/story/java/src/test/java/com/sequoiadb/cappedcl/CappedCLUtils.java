package com.sequoiadb.cappedcl;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.testcommon.CommLib;

public class CappedCLUtils {
    /**
     * 批量插入记录
     * 
     * @param
     * @return 无返回值
     * @throws Exception
     */
    public static void insertRecords( DBCollection cl, BSONObject insertObj,
            int recordNum ) {
        for ( int i = 0; i < recordNum; i++ ) {
            cl.insert( insertObj );
        }
    }

    /**
     * 生成随机记录长度
     * 
     * @param
     * @return int，记录长度
     * @throws Exception
     */
    public static int getRandomStringLength( int minLength, int maxLength ) {
        // int minLength = 1; // 100k
        // int maxLength = 10 * 1024; // 1M
        int stringLength = ( int ) ( minLength + Math.random() * maxLength );// [100k,1M]
        return stringLength;
    }

    /**
     * 检查固定集合主节点的logicalID是否符合预期
     * 
     * @param
     * @return boolean logicalID符合预期返回true，否则返回false
     * @throws Exception
     */
    public static boolean checkLogicalID( Sequoiadb db, String csName,
            String clName, int stringLength ) {

        // 固定集合只能在1个组上，获取组名
        ArrayList< String > groupNames = getCLGroupNames( db, csName, clName );
        ReplicaGroup rg = db.getReplicaGroup( groupNames.get( 0 ) );

        try ( Sequoiadb master = rg.getMaster().connect()) {
            DBCollection cl = master.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor cursor = cl.query( null, "{'_id':1}", "{'_id':1}", null );

            // 比较具体id值
            final int headLength = 55; // head length
            final int gap = 4; // 4字节对齐
            int recordLength = stringLength + headLength;
            int blockCount = 1; // 块数
            final int blockSize = 33554396; // 块大小，减去块头的大小，略小于32M
            long expectId = 0;
            while ( cursor.hasNext() ) {
                long actId = ( long ) cursor.getNext().get( "_id" );
                recordLength = ( 0 == recordLength % gap ) ? recordLength
                        : ( recordLength - recordLength % gap + gap );
                long nextRecordId = expectId + recordLength;
                // 跨块
                if ( nextRecordId > ( blockCount * blockSize ) ) {
                    expectId = blockCount * blockSize;
                    ++blockCount;
                }

                if ( expectId != actId ) {
                    System.out.println(
                            "logicalID in masterNode is wrong,expectId: "
                                    + expectId + "  actIdMaster: " + actId );
                    return false;
                }
                expectId = actId + recordLength;

            }
            cursor.close();
        }
        return true;
    }

    /**
     * 检查主备节点集合中记录是否一致
     * 
     * @param
     * @return boolean 主备节点一致则返回true，不一致则返回false
     * @throws Exception
     */
    public static boolean checkRecord( Sequoiadb db, String csName,
            String clName ) throws Exception {

        // 校验主备节点lsn
        isCLConsistency( db, csName, clName );

        // 获取集合所在的组
        ArrayList< String > groupNames = getCLGroupNames( db, csName, clName );
        for ( int i = 0; i < groupNames.size(); i++ ) {
            ReplicaGroup rg = db.getReplicaGroup( groupNames.get( i ) );
            ArrayList< String > nodeNames = getNodeNames( db,
                    groupNames.get( i ) );

            // 在第一个节点上的集合上查询数据，作为预期结果
            try ( Sequoiadb node0 = rg.getNode( ( String ) nodeNames.get( 0 ) )
                    .connect()) {
                DBCollection nodecl0 = node0.getCollectionSpace( csName )
                        .getCollection( clName );
                long count0 = nodecl0.getCount();
                DBCursor cursor0 = nodecl0.query( null, null, "{'_id':1}",
                        null );
                for ( int j = 1; j < nodeNames.size(); j++ ) {
                    try ( Sequoiadb node1 = rg
                            .getNode( ( String ) nodeNames.get( i ) )
                            .connect()) {
                        DBCollection nodecl1 = node1
                                .getCollectionSpace( csName )
                                .getCollection( clName );
                        long count1 = nodecl1.getCount();
                        // 比较记录数
                        if ( count0 != count1 ) {
                            System.out.println( "cl in nodeName:"
                                    + nodeNames.get( 0 ) + "getCount:" + count0
                                    + ",cl in nodeName:" + nodeNames.get( i )
                                    + "getCount:" + count1 );
                            return false;
                        }

                        // 比较记录
                        DBCursor cursor1 = nodecl1.query( null, null,
                                "{'_id':1}", null );
                        while ( cursor0.hasNext() && cursor1.hasNext() ) {
                            BSONObject record0 = cursor0.getNext();
                            BSONObject record1 = cursor1.getNext();

                            if ( !record0.equals( record1 ) ) {
                                System.out.println( "cl in nodeName:"
                                        + nodeNames.get( 0 ) + "record0:"
                                        + record0 + ",cl in nodeName:"
                                        + nodeNames.get( i ) + "record1:"
                                        + record1 );
                                return false;
                            }
                        }
                        cursor1.close();
                    }

                }
                cursor0.close();
            }

        }
        return true;
    }

    /**
     * 获取集合所在的组
     * 
     * @param
     * @return boolean 主备节点一致则返回true，不一致则返回false
     * @throws Exception
     */
    public static ArrayList< String > getCLGroupNames( Sequoiadb db,
            String csName, String clName ) {
        ArrayList< String > groupNames = new ArrayList< String >();
        DBCursor cursorSnapshot = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                "{Name:'" + csName + "." + clName + "'}", "{CataInfo:''}",
                null );

        BasicBSONList cataInfo = ( BasicBSONList ) cursorSnapshot.getNext()
                .get( "CataInfo" );
        cursorSnapshot.close();
        for ( int i = 0; i < cataInfo.size(); i++ )

        {
            BasicBSONObject record = ( BasicBSONObject ) cataInfo.get( i );
            String groupName = record.getString( "GroupName" );
            if ( !groupNames.isEmpty() && groupNames.contains( groupName ) ) {
                continue;
            }
            groupNames.add( groupName );
        }
        return groupNames;

    }

    /**
     * 获取组内所有节点的nodeName
     * 
     * @param
     * @return
     * @throws Exception
     */
    public static ArrayList< String > getNodeNames( Sequoiadb db,
            String groupName ) {
        ArrayList< String > nodeNames = new ArrayList< String >();
        ReplicaGroup rg = db.getReplicaGroup( groupName );
        BasicBSONList detail = ( BasicBSONList ) rg.getDetail().get( "Group" );
        String svcName = null;
        for ( int i = 0; i < detail.size(); i++ ) {
            BSONObject group = ( BSONObject ) detail.get( i );
            String hostName = ( String ) group.get( "HostName" );
            BasicBSONList services = ( BasicBSONList ) group.get( "Service" );
            for ( int j = 0; j < services.size(); j++ ) {
                BSONObject service = ( BSONObject ) services.get( i );
                if ( ( int ) service.get( "Type" ) == 0 ) {
                    svcName = ( String ) service.get( "Name" );
                    break;
                }
            }
            nodeNames.add( hostName + ":" + svcName );
        }
        return nodeNames;
    }

    /**
     * 检查主备节点集合CompleteLSN一致
     * 
     * @param cl
     * @return boolean 如果主节点CompleteLSN小于等于备节点CompleteLSN返回true,否则返回false
     * @throws Exception
     */
    private static boolean isCLConsistency( Sequoiadb db, String csName,
            String clName ) {

        boolean isConsistency = false;

        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clName );
        List< String > groupNames = FullTextDBUtils.getCLGroups( cl );
        for ( String groupName : groupNames ) {
            List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
            ReplicaGroup rg = db.getReplicaGroup( groupName );
            long completeLSN = -2;
            try ( Sequoiadb masterNode = rg.getMaster().connect()) {
                DBCursor cursor = masterNode.getSnapshot(
                        Sequoiadb.SDB_SNAP_SYSTEM, null, "{CompleteLSN: ''}",
                        null );
                if ( cursor.hasNext() ) {
                    completeLSN = ( long ) cursor.getNext()
                            .get( "CompleteLSN" );
                }
                cursor.close();
            }

            for ( String nodeName : nodeNames ) {
                isConsistency = false;
                try ( Sequoiadb nodeConn = rg.getNode( nodeName ).connect()) {
                    DBCursor cur = null;
                    long checkCompleteLSN = -2;
                    for ( int i = 0; i < 600; i++ ) {
                        cur = nodeConn.getSnapshot( Sequoiadb.SDB_SNAP_SYSTEM,
                                null, "{CompleteLSN: ''}", null );
                        if ( cur.hasNext() ) {
                            checkCompleteLSN = ( long ) cur.getNext()
                                    .get( "CompleteLSN" );
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
                }

            }
        }

        return isConsistency;
    }
}
