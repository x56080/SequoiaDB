package com.sequoiadb.sdbschedule.utils;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import io.restassured.response.Response;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;

import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class TestUtils {
    public static boolean isRenameCLExist( Sequoiadb sdb,
            String sourceCLName ) {
        String regex = "^" + sourceCLName + "_data_switch_bak_\\d+$";
        BSONObject match = new BasicBSONObject( "Name",
                new BasicBSONObject( "$regex", regex ) );
        try ( DBCursor cursor = sdb.getList( Sequoiadb.SDB_LIST_COLLECTIONS,
                match, null, null ) ;) {
            return cursor.hasNext();
        }
    }

    public static boolean compareSubCLRange( Sequoiadb db, String mainCLName,
            String subCLName, BSONObject expectLow, BSONObject expectUp ) {
        BSONObject mainCLCataInfo = getCataSnapshotByClName( db, mainCLName );
        if ( mainCLCataInfo == null ) {
            throw new RuntimeException( "the cl cataInfo not found, db="
                    + db.getRemoteAddress() + ", cl=" + mainCLCataInfo );
        }

        BasicBSONList cataInfo = ( BasicBSONList ) mainCLCataInfo
                .get( "CataInfo" );
        for ( Object obj : cataInfo ) {
            BSONObject info = ( BSONObject ) obj;
            String subCL = ( String ) info.get( "SubCLName" );
            if ( subCL.contains( subCLName ) ) {
                BSONObject low = ( BSONObject ) info.get( "LowBound" );
                BSONObject up = ( BSONObject ) info.get( "UpBound" );
                return expectLow.equals( low ) && expectUp.equals( up );
            }
        }

        return false;
    }

    public static boolean isMapped( Sequoiadb db, String clFullName ) {
        BSONObject cataInfo = getCataSnapshotByClName( db, clFullName );
        if ( cataInfo == null ) {
            throw new RuntimeException( "the cl cataInfo not found, db="
                    + db.getRemoteAddress() + ", cl=" + clFullName );
        }
        return cataInfo.containsField( "DataSourceID" );
    }

    public static boolean compareCataInfo( Sequoiadb sDB, Sequoiadb tDB,
            String mainClFullName ) {
        BSONObject sCataInfo = getCataSnapshotByClName( sDB, mainClFullName );
        if ( sCataInfo == null ) {
            throw new RuntimeException( "the cl cataInfo not found, db="
                    + sDB.getRemoteAddress() + ", cl=" + mainClFullName );
        }
        BSONObject tCataInfo = getCataSnapshotByClName( tDB, mainClFullName );
        if ( tCataInfo == null ) {
            throw new RuntimeException( "the cl cataInfo not found, db="
                    + tDB.getRemoteAddress() + ", cl=" + mainClFullName );
        }

        BasicBSONList sInfo = ( BasicBSONList ) sCataInfo.get( "CataInfo" );
        BasicBSONList tInfo = ( BasicBSONList ) tCataInfo.get( "CataInfo" );
        if ( sInfo.size() != tInfo.size() ) {
            return false;
        }
        return sInfo.equals( tInfo );
    }

    public static void attachCL( DBCollection mainCL, BSONObject lowBound,
            BSONObject upBound, String clFullName ) {
        BasicBSONObject attachOption = new BasicBSONObject();
        attachOption.put( "LowBound", lowBound );
        attachOption.put( "UpBound", upBound );
        mainCL.attachCollection( clFullName, attachOption );
    }

    public static boolean isRepairCheck( Sequoiadb db, String clFullName ) {

        BSONObject cataInfo = getCataSnapshotByClName( db, clFullName );
        if ( cataInfo == null ) {
            throw new RuntimeException( "the cl cataInfo not found, db="
                    + db.getRemoteAddress() + ", cl=" + clFullName );
        }
        if ( cataInfo.containsField( "RepairCheck" ) ) {
            return ( Boolean ) cataInfo.get( "RepairCheck" );
        }
        return false;
    }

    public static boolean compareCLMeta( Sequoiadb sDB, Sequoiadb tDB,
            String clFullName ) {
        BSONObject sCataInfo = getCataSnapshotByClName( sDB, clFullName );
        if ( sCataInfo == null ) {
            throw new RuntimeException( "the cl cataInfo not found, db="
                    + sDB.getRemoteAddress() + ", cl=" + clFullName );
        }
        BSONObject tCataInfo = getCataSnapshotByClName( tDB, clFullName );
        if ( tCataInfo == null ) {
            throw new RuntimeException( "the cl cataInfo not found, db="
                    + tDB.getRemoteAddress() + ", cl=" + clFullName );
        }

        BSONObject newOption = new BasicBSONObject();

        if ( isDiff( sCataInfo.get( "ReplSize" ),
                tCataInfo.get( "ReplSize" ) ) ) {
            newOption.put( "ReplSize", sCataInfo.get( "ReplSize" ) );
        }

        if ( isDiff( sCataInfo.get( "ConsistencyStrategy" ),
                tCataInfo.get( "ConsistencyStrategy" ) ) ) {
            newOption.put( "ConsistencyStrategy",
                    sCataInfo.get( "ConsistencyStrategy" ) );
        }

        if ( isDiff( sCataInfo.get( "ShardingKey" ),
                tCataInfo.get( "ShardingKey" ) ) ) {
            newOption.put( "ShardingKey", sCataInfo.get( "ShardingKey" ) );
        }

        if ( isDiff( sCataInfo.get( "ShardingType" ),
                tCataInfo.get( "ShardingType" ) ) ) {
            newOption.put( "ShardingType", sCataInfo.get( "ShardingType" ) );
        }

        if ( isDiff( sCataInfo.get( "Partition" ),
                tCataInfo.get( "Partition" ) ) ) {
            newOption.put( "Partition", sCataInfo.get( "Partition" ) );
        }

        if ( isDiff( sCataInfo.get( "AutoSplit" ),
                tCataInfo.get( "AutoSplit" ) ) ) {
            newOption.put( "AutoSplit", sCataInfo.get( "AutoSplit" ) );
        }

        if ( isDiff( sCataInfo.get( "EnsureShardingIndex" ),
                tCataInfo.get( "EnsureShardingIndex" ) ) ) {
            newOption.put( "EnsureShardingIndex",
                    sCataInfo.get( "EnsureShardingIndex" ) );
        }

        if ( isDiff( sCataInfo.get( "CompressionTypeDesc" ),
                tCataInfo.get( "CompressionTypeDesc" ) ) ) {
            if ( sCataInfo.containsField( "CompressionTypeDesc" ) ) {
                newOption.put( "CompressionType",
                        sCataInfo.get( "CompressionTypeDesc" ) );
                newOption.put( "Compressed", true );
            } else {
                newOption.put( "Compressed", false );
            }
        }

        if ( isDiff( sCataInfo.get( "AttributeDesc" ),
                tCataInfo.get( "AttributeDesc" ) ) ) {
            boolean sourceStrictDataMode = false;
            boolean targetStrictDataMode = false;
            boolean sourceAutoIdx = true;
            boolean targetAutoIdx = true;
            if ( sCataInfo.containsField( "AttributeDesc" ) ) {
                String attributeDesc = ( String ) tCataInfo
                        .get( "AttributeDesc" );
                if ( attributeDesc.contains( "StrictDataMode" ) ) {
                    sourceStrictDataMode = true;
                }
                if ( attributeDesc.contains( "NoIDIndex" ) ) {
                    sourceAutoIdx = false;
                }
            }

            if ( tCataInfo.containsField( "AttributeDesc" ) ) {
                String attributeDesc = ( String ) tCataInfo
                        .get( "AttributeDesc" );
                if ( attributeDesc.contains( "StrictDataMode" ) ) {
                    targetStrictDataMode = true;
                }
                if ( attributeDesc.contains( "NoIDIndex" ) ) {
                    targetAutoIdx = false;
                }
            }

            if ( sourceStrictDataMode != targetStrictDataMode ) {
                newOption.put( "StrictDataMode", sourceStrictDataMode );
            }

            if ( sourceAutoIdx != targetAutoIdx ) {
                newOption.put( "AutoIndexId", sourceAutoIdx );
            }
        }

        if ( newOption.isEmpty() ) {
            return true;
        }
        return false;
    }

    private static boolean isDiff( Object l, Object r ) {
        if ( l == null && r == null ) {
            return false;
        }

        if ( l == null || r == null ) {
            return true;
        }

        return !l.equals( r );
    }

    public static boolean compareAutoInc( Sequoiadb sDB, Sequoiadb tDB,
            String clName ) throws Exception {

        Set< String > sAutoIncFields = getAutoIncFields( sDB, clName );
        if ( sAutoIncFields == null || sAutoIncFields.isEmpty() ) {
            return true;
        }

        Set< String > targetAutoIncFields = getAutoIncFields( tDB, clName );

        if ( targetAutoIncFields == null || targetAutoIncFields.isEmpty() ) {
            return false;
        }

        if ( targetAutoIncFields.size() < sAutoIncFields.size() ) {
            return false;
        }

        for ( String sourceAutoIncField : sAutoIncFields ) {
            if ( !targetAutoIncFields.contains( sourceAutoIncField ) ) {
                return false;
            }
        }

        return true;
    }

    public static Set< String > getAutoIncFields( Sequoiadb sdb,
            String csClName ) {
        BSONObject clCataInfo = getCataSnapshotByClName( sdb, csClName );
        if ( clCataInfo == null ) {
            throw new RuntimeException( "the cl cataInfo not found db="
                    + sdb.getRemoteAddress() + ", cl=" + csClName );
        }
        return getAutoIncFields( clCataInfo );
    }

    public static BSONObject getCataSnapshotByClName( Sequoiadb sdb,
            String clName ) {
        try ( DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                new BasicBSONObject( "Name", clName ), null, null )) {
            if ( cursor.hasNext() ) {
                return cursor.getNext();
            }
            return null;
        }
    }

    public static Set< String > getAutoIncFields( BSONObject clCataInfo ) {
        Set< String > set = new HashSet<>();
        if ( !clCataInfo.containsField( "AutoIncrement" ) ) {
            return set;
        }
        BasicBSONList autoIncrementList = ( BasicBSONList ) clCataInfo
                .get( "AutoIncrement" );
        if ( autoIncrementList == null || autoIncrementList.isEmpty() ) {
            return set;
        }

        for ( Object o : autoIncrementList ) {
            BSONObject autoIncInfo = ( BSONObject ) o;
            String field = ( String ) autoIncInfo.get( "Field" );
            set.add( field );
        }
        return set;
    }

    public static boolean compareIndex( DBCollection sCL, DBCollection tCL ) {
        try ( DBCursor cursor = sCL.getIndexes()) {
            while ( cursor.hasNext() ) {
                BSONObject indexObj = cursor.getNext();
                BSONObject indexDef = ( BSONObject ) indexObj.get( "IndexDef" );
                String name = ( String ) indexDef.get( "name" );
                if ( tCL.isIndexExist( name ) ) {
                    BSONObject sourceKey = ( BSONObject ) indexDef.get( "key" );
                    BSONObject targetIndexInfo = tCL.getIndexInfo( name );
                    BSONObject targetIndexDef = ( BSONObject ) targetIndexInfo
                            .get( "IndexDef" );
                    BSONObject targetKey = ( BSONObject ) targetIndexDef
                            .get( "key" );
                    if ( sourceKey.equals( targetKey ) ) {
                        continue;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            }
        }
        return true;
    }

    public static void waitTaskNum( String id, int taskNum ) {
        waitTaskNum( id, taskNum, 60 );
    }

    public static void waitTaskNum( String id, int taskNum, int maxWait ) {
        do {
            Response resp = BusinessApiFactory.Schedule.listTasks( id, 0, -1,
                    null, null );
            BasicBSONList taskList = ( BasicBSONList ) BsonUtils
                    .fromResponse( resp );
            if ( taskList.size() >= taskNum ) {
                return;
            }

            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                // ignore
            }
        } while ( --maxWait > 0 );

        throw new RuntimeException( "Wait schedule tasks num timeout" );
    }

    public static void waitFinish( String id, int finishNum ) {

        waitFinish( id, finishNum, 120 );
    }

    public static void waitFinish( String id, int finishNum, int maxWait ) {
        do {
            Response resp = BusinessApiFactory.Schedule.listTasks( id, 0, -1,
                    new BasicBSONObject( "running_flag", 3 ), null );
            BasicBSONList taskList = ( BasicBSONList ) BsonUtils
                    .fromResponse( resp );
            if ( taskList.size() >= finishNum ) {
                return;
            }

            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                // ignore
            }
        } while ( --maxWait > 0 );

        throw new RuntimeException( "Wait schedule tasks finish timeout" );
    }

    public static void waitAllFinish( String id ) {
        waitAllFinish( id, 120 );
    }

    public static void waitAllFinish( String id, int maxWait ) {
        List< Integer > notFinishFlags = new ArrayList<>();
        notFinishFlags.add( 1 );
        notFinishFlags.add( 2 );
        BSONObject matcher = new BasicBSONObject( "running_flag",
                new BasicBSONObject( "$in", notFinishFlags ) );
        while ( maxWait-- > 0 ) {
            Response resp = BusinessApiFactory.Schedule.listTasks( id, 0, -1,
                    matcher, null );
            BasicBSONList taskList = ( BasicBSONList ) BsonUtils
                    .fromResponse( resp );
            if ( taskList.size() == 0 ) {
                return;
            }
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                // ignore
            }
        }
        throw new RuntimeException( "Wait schedule all tasks finish timeout" );
    }

    public static CollectionSpace initCS(Sequoiadb sdb, String csName) {
        cleanCS(sdb, csName);
        return sdb.createCollectionSpace( csName );
    }

    public static void cleanCS(Sequoiadb sdb, String csName) {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
    }

    public static boolean compareRecord( DBCollection sCL, DBCollection tCL ) {
        long sCount = sCL.getCount();
        long tCount = tCL.getCount();
        if ( sCount != tCount ) {
            System.out.println(
                    "Source and target collection record count not same, source count="
                            + sCount + ", target count=" + tCount );
            return false;
        }

        BSONObject sort = new BasicBSONObject( "_id", 1 );

        try ( DBCursor sCursor = sCL.query( null, null, sort, null ) ;
                DBCursor tCursor = tCL.query( null, null, sort, null ) ;) {
            BSONObject sR = sCursor.hasNext() ? sCursor.getNext() : null;
            BSONObject tR = tCursor.hasNext() ? tCursor.getNext() : null;
            while ( sR != null && tR != null ) {
                if ( !sR.equals( tR ) ) {
                    System.out.println(
                            "Source and target collection record not same, source record="
                                    + sR + ", target record=" + tR );
                    return false;
                }

                sR = sCursor.hasNext() ? sCursor.getNext() : null;
                tR = tCursor.hasNext() ? tCursor.getNext() : null;
            }

            return sR == null;
        }
    }

    public static boolean compareLobData( DBCollection sCL, DBCollection tCL )
            throws Exception {
        BSONObject sSort = new BasicBSONObject( "CreateTime", 1 );
        sSort.put( "Oid", 1 );
        BSONObject tSort = new BasicBSONObject( "UserData.CreateTime", 1 );
        tSort.put( "Oid", 1 );
        try ( DBCursor sCursor = sCL.listLobs( null, null, sSort, null, 0,
                -1 ) ;
                DBCursor tCursor = tCL.listLobs( null, null, tSort, null, 0,
                        -1 ) ;) {
            BSONObject sourceLob = sCursor.hasNext() ? sCursor.getNext() : null;
            BSONObject targetLob = tCursor.hasNext() ? tCursor.getNext() : null;

            while ( sourceLob != null && targetLob != null ) {
                CompareLobResult compareLobResult = TestUtils.compareLob( sCL,
                        tCL, sourceLob, targetLob );
                if ( compareLobResult == CompareLobResult.SAME ) {
                    sourceLob = sCursor.hasNext() ? sCursor.getNext() : null;
                    targetLob = tCursor.hasNext() ? tCursor.getNext() : null;
                } else if ( compareLobResult == CompareLobResult.NOT_USER_DATA ) {
                    targetLob = tCursor.hasNext() ? tCursor.getNext() : null;
                } else {
                    System.out.println( "LOB data not same, sourceLobInfo="
                            + sourceLob + ", targetLobInfo=" + targetLob );
                    return false;
                }
            }

            return sourceLob == null;
        }
    }

    public enum CompareLobResult {
        SAME, LOB_MODIFY, SOURCE_NEW, NOT_USER_DATA, TARGET_RESIDUE;
    }

    public static CompareLobResult compareLob( DBCollection sCL,
            DBCollection tCL, BSONObject sourceLob, BSONObject targetLob )
            throws Exception {
        ObjectId sId = ( ObjectId ) sourceLob.get( "Oid" );
        ObjectId tId = ( ObjectId ) targetLob.get( "Oid" );
        if ( !sId.equals( tId ) ) {
            BSONObject userData = ( BSONObject ) targetLob.get( "UserData" );
            if ( userData == null ) {
                // 没有自定义元数据，不是迁移过来的LOB，不管
                return CompareLobResult.NOT_USER_DATA;
            }

            BSONTimestamp sCreateTime = ( BSONTimestamp ) sourceLob
                    .get( "CreateTime" );
            BSONTimestamp tCreateTime = ( BSONTimestamp ) userData
                    .get( "CreateTime" );
            if ( tCreateTime == null ) {
                // 没有自定义元数据，不是迁移过来的LOB，不管
                return CompareLobResult.NOT_USER_DATA;
            }

            if ( compareTime( sCreateTime, tCreateTime ) <= 0 ) {
                // 源端新增，目标端缺失
                return CompareLobResult.SOURCE_NEW;
            } else {
                // 目标端残留，源端缺失
                return CompareLobResult.TARGET_RESIDUE;
            }
        } else {
            // Oid 相等，继续比较其他属性
            BSONObject userData = ( BSONObject ) targetLob.get( "UserData" );
            if ( userData == null ) {
                // 没有自定义元数据，直接视为被修改
                return CompareLobResult.LOB_MODIFY;
            }

            BSONTimestamp sCreateTime = ( BSONTimestamp ) sourceLob
                    .get( "CreateTime" );
            BSONTimestamp tCreateTime = ( BSONTimestamp ) userData
                    .get( "CreateTime" );
            if ( tCreateTime == null ) {
                // 没有自定义元数据，直接视为被修改
                return CompareLobResult.LOB_MODIFY;
            }
            if ( !sCreateTime.equals( tCreateTime ) ) {
                // 创建时间不相等，LOB 视为被修改
                return CompareLobResult.LOB_MODIFY;
            }

            BSONTimestamp sModificationTime = ( BSONTimestamp ) sourceLob
                    .get( "ModificationTime" );
            BSONTimestamp tModificationTime = ( BSONTimestamp ) userData
                    .get( "ModificationTime" );
            if ( tModificationTime == null ) {
                // 没有自定义元数据，直接视为被修改
                return CompareLobResult.LOB_MODIFY;
            }
            if ( !sModificationTime.equals( tModificationTime ) ) {
                // 修改时间不相等，LOB 视为被修改
                return CompareLobResult.LOB_MODIFY;
            }

            Long sSize = ( ( Number ) sourceLob.get( "Size" ) ).longValue();
            Long tSize = ( ( Number ) targetLob.get( "Size" ) ).longValue();
            if ( !sSize.equals( tSize ) ) {
                return CompareLobResult.LOB_MODIFY;
            }

            if ( !checkDataSame( sCL, tCL, sId ) ) {
                return CompareLobResult.LOB_MODIFY;
            }

            return CompareLobResult.SAME;
        }
    }

    public static boolean checkDataSame( DBCollection sCL, DBCollection tCL,
            ObjectId lobID ) throws Exception {
        try ( DBLob sLob = sCL.openLob( lobID ) ;
                DBLob tLob = tCL.openLob( lobID )) {
            MessageDigest sDigest = MessageDigest.getInstance( "MD5" );
            MessageDigest tDigest = MessageDigest.getInstance( "MD5" );

            byte[] sBuffer = new byte[ 1024 ];
            byte[] tBuffer = new byte[ 1024 ];
            int sLen;
            int tLen;

            while ( true ) {
                sLen = sLob.read( sBuffer );
                tLen = tLob.read( tBuffer );

                if ( sLen == -1 && tLen == -1 ) {
                    break;
                }

                if ( sLen != tLen ) {
                    return false;
                }

                sDigest.update( sBuffer, 0, sLen );
                tDigest.update( tBuffer, 0, tLen );
            }

            String sMd5 = bytesToHexStr( sDigest.digest() );
            String tMd5 = bytesToHexStr( tDigest.digest() );

            return sMd5.equals( tMd5 );
        } catch ( Exception e ) {
            throw new Exception(
                    "failed to check lob data same, lobId=" + lobID, e );
        }
    }

    public static String bytesToHexStr( byte[] b ) {
        StringBuilder buf = new StringBuilder();
        for ( byte value : b ) {
            int x = value & 0xFF;
            String s = Integer.toHexString( x ).toUpperCase();
            if ( s.length() == 1 ) {
                buf.append( "0" );
            }

            buf.append( s );
        }

        return buf.toString();
    }

    public static int compareTime( BSONTimestamp time1, BSONTimestamp time2 ) {
        if ( time1.getTime() < time2.getTime() ) {
            return -1;
        } else if ( time1.getTime() > time2.getTime() ) {
            return 1;
        } else {
            return Integer.compare( time1.getInc(), time2.getInc() );
        }
    }
}
