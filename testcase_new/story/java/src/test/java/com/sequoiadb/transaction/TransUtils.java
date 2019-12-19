package com.sequoiadb.transaction;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 
 * @description Utils for this package class
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class TransUtils extends SdbTestBase {

    public static final int FLG_INSERT_CONTONDUP = 0x00000001;
    /**
     * delayTime 线程延时启动时间，线程Thread.sleep(事务等锁超时时间-20s)
     */
    public static final int delayTime = ( SdbTestBase.timeOutLen - 20 ) * 1000;

    public static CollectionSpace createCS( String csName, Sequoiadb db )
            throws BaseException {
        CollectionSpace tmp = null;
        try {
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
            tmp = db.createCollectionSpace( csName );

        } catch ( BaseException e ) {
            throw e;
        }
        return tmp;
    }

    public static CollectionSpace createCS( String csName, Sequoiadb db,
            String option ) throws BaseException {
        CollectionSpace tmp = null;
        try {
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
            tmp = db.createCollectionSpace( csName,
                    ( BSONObject ) JSON.parse( option ) );

        } catch ( BaseException e ) {
            throw e;
        }
        return tmp;
    }

    public static Domain createDomain( Sequoiadb sdb, String name,
            ArrayList< String > groupArr, int size, boolean autoSplit )
            throws BaseException {
        Domain domain = null;
        try {
            if ( sdb.isDomainExist( name ) ) {
                domain = sdb.getDomain( name );
            } else {
                StringBuilder groups = new StringBuilder();
                String option = new String();
                for ( int i = 0; i < groupArr.size() && i < size; i++ ) {
                    groups.append( "\"" ).append( groupArr.get( i ) )
                            .append( "\"," );
                }
                groups.deleteCharAt( groups.length() - 1 );
                groups.insert( 0, "[" );
                groups.append( "]" );
                if ( autoSplit ) {
                    option = "{\"Groups\":" + groups + ",\"AutoSplit\":true}";
                } else {
                    option = "{\"Groups\":" + groups + ",\"AutoSplit\":false}";
                }
                domain = sdb.createDomain( name,
                        ( BSONObject ) JSON.parse( option ) );
            }

        } catch ( BaseException e ) {
            throw e;
        }
        return domain;
    }

    public static boolean getDatabaseSnapshot( Sequoiadb db,
            String groupName ) {
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                "{RawData:true, GroupName:'" + groupName + "'}",
                "{TransInfo:''}", null );
        while ( cursor.hasNext() ) {
            BSONObject info = ( BSONObject ) cursor.getNext();
            BSONObject transInfo = ( BSONObject ) info.get( "TransInfo" );
            int transCount = ( int ) transInfo.get( "TotalCount" );
            Long transBeginLSN = ( Long ) transInfo.get( "BeginLSN" );

            if ( transCount != 0 || transBeginLSN != -1 ) {
                System.out.println( "transCount:" + transCount
                        + "\ntransBeginLSN:" + transBeginLSN );
                return false;
            }
        }
        return true;
    }

    public static DBCollection createCL( String clName, CollectionSpace cs,
            String option ) throws BaseException {
        DBCollection tmp = null;
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            tmp = cs.createCollection( clName,
                    ( BSONObject ) JSON.parse( option ) );
        } catch ( BaseException e ) {
            throw e;
        }
        return tmp;
    }

    // 检查某集合是否仅含一个dest记录
    public static boolean isCollectionContainThisJSON( DBCollection cl,
            String dest ) throws BaseException {
        BSONObject bobj = ( BSONObject ) JSON.parse( dest );
        ArrayList< Object > resaults = new ArrayList< Object >();
        DBCursor dc = null;
        try {
            dc = cl.query( bobj, null, null, null );
            while ( dc.hasNext() ) {
                resaults.add( dc.getNext() );
            }
            if ( resaults.size() != 1 ) {
                return false;
            }
            BSONObject actual = ( BSONObject ) resaults.get( 0 );
            actual.removeField( "_id" );
            bobj.removeField( "_id" );
            if ( bobj.equals( actual ) ) {
                return true;
            } else {
                return false;
            }
        } catch ( BaseException e ) {
            throw e;
        }
    }

    public static String getKeyStack( Exception e, Object classObj ) {
        StringBuffer stackBuffer = new StringBuffer();
        StackTraceElement[] stackElements = e.getStackTrace();
        for ( int i = 0; i < stackElements.length; i++ ) {
            if ( stackElements[ i ].toString()
                    .contains( classObj.getClass().getName() ) ) {
                stackBuffer.append( stackElements[ i ].toString() )
                        .append( "\r\n" );
            }
        }
        String str = stackBuffer.toString();
        return str.substring( 0, str.length() - 2 );
    }

    public static ArrayList< String > getGroupName( Sequoiadb sdb,
            String csName, String clName ) throws BaseException {
        DBCursor dbc = null;
        ArrayList< String > resault = new ArrayList< String >();
        try {
            ArrayList< String > groups = CommLib.getDataGroupNames( sdb );
            dbc = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + csName + "." + clName + "\"}", null, null );
            BasicBSONList list = null;
            if ( dbc.hasNext() ) {
                list = ( BasicBSONList ) dbc.getNext().get( "CataInfo" );
            } else {
                return null;
            }
            String srcGroupName = ( String ) ( ( BSONObject ) list.get( 0 ) )
                    .get( "GroupName" );
            resault.add( srcGroupName );
            if ( groups.size() < 2 ) {
                return resault;
            }
            String destGroupName;
            if ( srcGroupName.equals( groups.get( 0 ) ) )
                destGroupName = groups.get( 1 );
            else
                destGroupName = groups.get( 0 );
            resault.add( destGroupName );
            return resault;
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }
    }

    public static ArrayList< BSONObject > getReadActList( DBCursor cursor )
            throws BaseException {
        ArrayList< BSONObject > actRList = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            actRList.add( record );
        }
        cursor.close();
        return actRList;
    }

    public static ArrayList< BSONObject > insertDatas( DBCollection cl,
            int startId, int endId, int insertValue ) throws BaseException {
        ArrayList< BSONObject > insertDatas = new ArrayList< BSONObject >();
        for ( int i = startId; i < endId; i++ ) {
            insertDatas.add( ( BSONObject ) JSON.parse(
                    "{_id:" + i + ",a:" + insertValue + ",b:" + i + "}" ) );
        }
        cl.insert( insertDatas );
        return insertDatas;
    }

    public static ArrayList< BSONObject > insertRandomDatas( DBCollection cl,
            int startId, int endId ) throws BaseException {
        ArrayList< BSONObject > insertDatas = new ArrayList< BSONObject >();
        ArrayList< BSONObject > expDatas = new ArrayList< BSONObject >();
        for ( int i = startId; i < endId; i++ ) {
            BSONObject data = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ",a:" + i + ",b:" + i + "}" );
            insertDatas.add( data );
            expDatas.add( data );
        }
        Collections.shuffle( insertDatas );
        cl.insert( insertDatas );
        return expDatas;
    }

    public static boolean getReadActList( DBCursor cursor, StringBuilder expRes,
            int pos ) throws BaseException {
        String prefix = expRes.toString();
        int diff = 1;
        if ( pos >= 10 ) {
            diff = 2;
        }
        while ( cursor.hasNext() ) {
            BasicBSONObject record = ( BasicBSONObject ) cursor.getNext();
            String filedA = record.getString( "a" );
            if ( filedA.indexOf( prefix ) == -1 ) {
                return false;
            }

            if ( filedA.length() - prefix.length() != diff ) {
                return false;
            }

            if ( Integer
                    .parseInt( filedA.substring( prefix.length() ) ) != pos ) {
                return false;
            }

            pos++;
        }
        return true;
    }

    public static ArrayList< BSONObject > getUpdateDatas( int startId,
            int endId, int updateValue ) {
        ArrayList< BSONObject > updateDatas = new ArrayList< BSONObject >();
        for ( int i = startId; i < endId; i++ ) {
            updateDatas.add( ( BSONObject ) JSON.parse(
                    "{_id:" + i + ",a:" + updateValue + ",b:" + i + "}" ) );
        }
        return updateDatas;
    }

    public static ArrayList< BSONObject > getIncDatas( int startId, int endId,
            int incValue ) {
        ArrayList< BSONObject > incDatas = new ArrayList< BSONObject >();
        for ( int i = startId; i < endId; i++ ) {
            incDatas.add( ( BSONObject ) JSON.parse( "{_id:" + i + ",a:"
                    + ( incValue + i ) + ",b:" + i + "}" ) );
        }
        return incDatas;
    }

    /**
     * 构造复合索引所需要的数据 如：a:0, b:0 a:1, b:0 a:1, b:1 a:1, b:2 ... a:2, b:2 a:3, b:0
     * a:3, b:1 ... a 为偶数时，a 和 b 一致 a 为奇数时，有多条记录 a 相等，b 不相等 aStart a 的起始值，aEnd a
     * 的结束值，bStart a 为奇数时 b 的起始值，bEnd a 为奇数时 b 的结束值 返回 list 长度 为 11*(aEnd -
     * aStart)/2
     * 
     * @return
     */
    public static List< BSONObject > getCompositeRecords( int aStart, int aEnd,
            int bStart, int bEnd ) {
        int a = 0;
        int b = 0;
        int id = ( aStart / 2 ) * 11 + aStart % 2;
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = aStart; i < aEnd; i++ ) {
            if ( i % 2 == 0 ) {
                a = i;
                b = i;
                BSONObject object = ( BSONObject ) JSON.parse(
                        "{_id:" + id++ + ", a:" + a + ", b:" + b + "}" );
                records.add( object );
            } else {
                for ( int j = bStart; j < bEnd; j++ ) {
                    a = i;
                    b = j;
                    BSONObject object = ( BSONObject ) JSON.parse(
                            "{_id:" + id++ + ", a:" + a + ", b:" + b + "}" );
                    records.add( object );
                }
            }
        }
        Collections.shuffle( records );
        return records;
    }

    /**
     * 排序 参数 key ：true b 字段正序排序，false 逆序
     * 
     * @param records
     */
    public static void sortCompositeRecords( List< BSONObject > records,
            final boolean key ) {

        Collections.sort( records, new Comparator< BSONObject >() {
            @Override
            public int compare( BSONObject obj1, BSONObject obj2 ) {
                if ( ( int ) obj1.get( "a" ) == ( int ) obj2.get( "a" ) ) {
                    if ( key ) {
                        return ( int ) obj1.get( "b" )
                                - ( int ) obj2.get( "b" );
                    } else {
                        return -( ( int ) obj1.get( "b" )
                                - ( int ) obj2.get( "b" ) );
                    }
                } else {
                    return ( int ) obj1.get( "a" ) - ( int ) obj2.get( "a" );
                }
            }
        } );
    }

    /**
     * 创建主子表和切分表，切分表自动切分，主表attach平普通表和切分表，切分子表为自动切分，分区键为_id
     * 
     * @param sdb
     * @param csName
     * @param hashCLName
     * @param mainCLName
     * @param subCLName1
     *            子表名1
     * @param subCLName2
     *            子表名2
     * @param sep
     *            主表的切分范围为 (min - sep)(sep - max)
     */
    public static void createCLs( Sequoiadb sdb, String csName,
            String hashCLName, String mainCLName, String subCLName1,
            String subCLName2, int sep ) {
        createHashCL( sdb, csName, hashCLName );
        createMainCL( sdb, csName, mainCLName, subCLName1, subCLName2, sep );
    }

    /**
     * 创建切分表
     * 
     * @param sdb
     * @param csName
     * @param hashCLName
     */
    public static void createHashCL( Sequoiadb sdb, String csName,
            String hashCLName ) {
        sdb.getCollectionSpace( csName ).createCollection( hashCLName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{_id:1}, ShardingType:'hash', AutoSplit:true}" ) );
    }

    /**
     * 创建主子表
     * 
     * @param sdb
     * @param csName
     * @param mainCLName
     * @param subCLName1
     *            子表名1
     * @param subCLName2
     *            子表名2
     * @param sep
     *            主表的切分范围为(min - sep)(sep - max)
     */
    public static void createMainCL( Sequoiadb sdb, String csName,
            String mainCLName, String subCLName1, String subCLName2, int sep ) {
        DBCollection mainCL = sdb.getCollectionSpace( csName )
                .createCollection( mainCLName, ( BSONObject ) JSON.parse(
                        "{ShardingKey:{_id:1}, ShardingType:'range', IsMainCL:true}" ) );
        sdb.getCollectionSpace( csName ).createCollection( subCLName1 );
        sdb.getCollectionSpace( csName ).createCollection( subCLName2,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{_id:1}, ShardingType:'hash', AutoSplit:true}" ) );
        mainCL.attachCollection( csName + "." + subCLName1,
                ( BSONObject ) JSON
                        .parse( "{LowBound:{_id:{'$minKey':1}}, UpBound:{_id:"
                                + sep + "}}" ) );
        mainCL.attachCollection( csName + "." + subCLName2,
                ( BSONObject ) JSON.parse( "{LowBound:{_id:" + sep
                        + "}, UpBound:{_id:{'$maxKey':1}}}" ) );
    }

    /**
     * 查询记录
     * 
     * @param cl
     *            集合对象
     * @param orderBy
     *            排序规则
     * @param hint
     *            排序规则
     * @return 返回记录 BSONObject 的 List
     */
    public static List< BSONObject > queryToBSONList( DBCollection cl,
            String orderBy, String hint ) {
        return queryToBSONList( cl, null, null, orderBy, hint );
    }

    /**
     * 查询记录
     * 
     * @param cl
     *            集合对象
     * @param matcher
     *            匹配规则
     * @param selector
     *            选择规则
     * @param orderBy
     *            排序规则
     * @param hint
     *            索引条件
     * @return 返回记录 BSONObject 的 List
     */
    public static List< BSONObject > queryToBSONList( DBCollection cl,
            String matcher, String selector, String orderBy, String hint ) {
        List< BSONObject > records = null;
        DBCursor cursor = cl.query( matcher, selector, orderBy, hint );
        records = getReadActList( cursor );
        return records;
    }

    /**
     * 查询并检查记录正确性
     * 
     * @param cl
     * @param hint
     * @param expList
     *            预期结果，记录 BSONObject 的 List
     */
    public static void queryAndCheck( DBCollection cl, String hint,
            List< BSONObject > expList ) {
        queryAndCheck( cl, null, null, null, hint, expList );
    }

    /**
     * 查询并检查记录正确性
     * 
     * @param cl
     * @param orderBy
     * @param hint
     * @param expList
     *            预期结果，记录 BSONObject 的 List
     */
    public static void queryAndCheck( DBCollection cl, String orderBy,
            String hint, List< BSONObject > expList ) {
        queryAndCheck( cl, null, null, orderBy, hint, expList );
    }

    /**
     * 查询并检查记录正确性
     * 
     * @param cl
     * @param matcher
     * @param selector
     * @param orderBy
     * @param hint
     * @param expList
     *            预期结果，记录 BSONObject 的 List
     */
    public static void queryAndCheck( DBCollection cl, String matcher,
            String selector, String orderBy, String hint,
            List< BSONObject > expList ) {
        // 由于matchBlockingMethod只能判断一个接口，大量的用例均调用了该接口，因此先query再count！！！！
        List< BSONObject > actList = queryToBSONList( cl, matcher, selector,
                orderBy, hint );
        Assert.assertEquals( actList, expList );

        // 该测试点是校验count接口的，不能够删除
        BSONObject matcherBSON = ( BSONObject ) JSON.parse( matcher );
        BSONObject hintBSON = ( BSONObject ) JSON.parse( hint );
        long actCount = cl.getCount( matcherBSON, hintBSON );
        long expCount = expList.size();
        Assert.assertEquals( actCount, expCount );
    }

    /**
     * 对 List<BSONObject> 使用 _id 字段进行排序
     * 
     * @param list
     */
    public static void sortList( List< BSONObject > list ) {
        sortList( list, null );
    }

    /**
     * 对 List<BSONObject> 进行排序，sortField 为空时默认对 _id 字段排序
     * 
     * @param list
     * @param sortField
     */
    public static void sortList( List< BSONObject > list, String sortField ) {
        if ( sortField == null ) {
            sortField = "_id";
        }
        Collections.sort( list, new sortList( sortField ) );
    }

    /**
     * 创建唯一索引预期报 -38，rcuserbs 模式预期报 -334
     * 
     * @param cl
     * @param idxName
     * @param idxKey
     */
    public static void createUniIdxErr( DBCollection cl, String idxName,
            String idxKey ) {
        try {
            cl.createIndex( idxName, idxKey, true, false );
            Assert.fail( "CREATE IDX SHOULD THROW ERR" );
        } catch ( BaseException e ) {
            if ( "rcuserbs".equals( SdbTestBase.testGroup ) ) {
                if ( -334 != e.getErrorCode() ) {
                    throw e;
                }
            } else {
                if ( -38 != e.getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void dropCS( Sequoiadb db, String csName )
            throws InterruptedException {
        for ( int i = 0; i < 30; i++ ) {
            if ( db.isCollectionSpaceExist( csName ) ) {
                try {
                    db.dropCollectionSpace( csName );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -147 ) {
                        throw e;
                    }

                }
            } else {
                break;
            }
            Thread.sleep( 1000 );
        }

    }

    public static boolean isLsnConsistency( Sequoiadb sdb, String groupName ) {
        boolean isConsistency;
        int eachSleepTime = 1000;
        int maxSleetTime = 600000;
        int alreadySleepTime = 0;
        List< String > nodeUrls = CommLib.getNodeAddress( sdb, groupName );

        do {
            isConsistency = true;
            long lsnOfPrevNode = -1;
            int versionOfPrevNode = 0;
            for ( String nodeUrl : nodeUrls ) {
                try ( Sequoiadb dataDB = new Sequoiadb( nodeUrl, "", "" ) ;) {
                    DBCursor cursor = dataDB.getSnapshot(
                            Sequoiadb.SDB_SNAP_DATABASE, null,
                            "{CurrentLSN:null, CompleteLSN:null}", null );
                    while ( cursor.hasNext() ) {
                        BasicBSONObject doc = ( BasicBSONObject ) cursor
                                .getNext();
                        long lsnOfCurNode = doc.getLong( "CompleteLSN" );
                        int versionOfCurNode = ( ( BasicBSONObject ) doc
                                .get( "CurrentLSN" ) ).getInt( "Version" );
                        if ( lsnOfPrevNode == -1 ) {
                            lsnOfPrevNode = lsnOfCurNode;
                            versionOfPrevNode = versionOfCurNode;
                            continue;
                        } else if ( lsnOfPrevNode != lsnOfCurNode
                                || versionOfPrevNode != versionOfCurNode ) {
                            isConsistency = false;
                            break;
                        }
                    }
                    cursor.close();
                }
            }

            if ( isConsistency ) {
                break;
            }

            if ( alreadySleepTime >= maxSleetTime ) {
                break;
            }

            try {
                Thread.sleep( eachSleepTime );
                alreadySleepTime += eachSleepTime;
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        } while ( true );

        return isConsistency;
    }

    // 获取事务ID
    public static String getTransactionID( Sequoiadb db ) {
        // 在已开启事务的会话上，执行一个无关的查询，以获取当前会话上的事务id
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( reservedCL );
        DBCursor cursor = cl.query();
        while ( cursor.hasNext() ) {
            cursor.getNext();
        }
        cursor.close();
        String transactionID = ( String ) db
                .getSnapshot( Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, "",
                        "{TransactionID:''}", "" )
                .getCurrent().get( "TransactionID" );
        return transactionID;
    }

    // 判断是否是否在等锁
    public static boolean isTransWaitLock( Sequoiadb db,
            String transactionID ) {
        // 避免线程未启动
        try {
            Thread.sleep( 500 );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
        boolean isTransWaitLock = false;
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSACTIONS,
                "{TransactionID:'" + transactionID + "'}", "{WaitLock:\"\"}",
                "" );
        while ( cursor.hasNext() ) {
            BSONObject waitLock = ( BSONObject ) cursor.getNext()
                    .get( "WaitLock" );
            if ( !waitLock.isEmpty() ) {
                isTransWaitLock = true;
            }
        }
        cursor.close();
        return isTransWaitLock;
    }
}

/**
 * 
 * @description 排序类
 * @author yinzhen
 * @date 2019年7月19日
 */
class sortList implements Comparator< BSONObject > {
    private String sortField;

    sortList( String soerField ) {
        this.sortField = soerField;
    }

    @Override
    public int compare( BSONObject o1, BSONObject o2 ) {
        String field1 = String.valueOf( o1.get( sortField ) );
        String field2 = String.valueOf( o2.get( sortField ) );
        return field1.compareTo( field2 );
    }

}
