/**
 * 
 */
/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:TransRBS.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2020年1月31日下午3:07:39
 *  @version 1.00
 */
package com.sequoiadb.transaction.common;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * @author zhaoyu
 *
 */

public class TransRBS {
    /**
     * 循环次数
     */
    public static int loopNum = 20;

    public static List< BSONObject > insertDatas( DBCollection cl ) {
        List< BSONObject > records = new ArrayList<>();
        StringBuilder sb = new StringBuilder();
        for ( int i = 0; i < 100; i++ ) {
            sb.append( "a" );
        }

        for ( int i = 0; i < 10000; i++ ) {
            records.add( ( BSONObject ) JSON.parse( "{_id:" + i + ",a:" + i
                    + ",b:" + i + ",c:'" + sb + "'}" ) );
        }
        List< BSONObject > expList = new ArrayList<>( records );
        cl.insert( records );
        return expList;
    }

    public static ArrayList< BSONObject > insertRandomLengthRecords(
            DBCollection cl, int insertNum, int minStringLength,
            int maxStringLenth ) throws BaseException {
        ArrayList< BSONObject > insertDatas = new ArrayList<>();
        ArrayList< BSONObject > expDatas = new ArrayList<>();
        for ( int i = 0; i < insertNum; i++ ) {
            StringBuilder sb = new StringBuilder();
            int stringLength = new Random().nextInt( maxStringLenth )
                    + minStringLength;
            for ( int j = 0; j < stringLength; j++ ) {
                sb.append( "a" );
            }
            insertDatas.add( ( BSONObject ) JSON.parse( "{_id:" + i + ",a:" + i
                    + ",b:" + i + ",c:'" + sb + "'}" ) );
        }
        expDatas.addAll( insertDatas );
        Collections.shuffle( insertDatas );
        cl.insert( insertDatas );
        return expDatas;
    }

    public static void genMultiRBSCL( DBCollection cl, int loopNum ) {
        Sequoiadb db = cl.getSequoiadb();
        for ( int i = 0; i < loopNum; i++ ) {
            try {
                db.beginTransaction();
                cl.update( null, "{$inc:{a:1}}", "{'':'a'}" );
                db.commit();
                cl.update( null, "{$inc:{a:1}}", "{'':'a'}" );
                System.out.println( "transaction update exec loop num:" + i );
            } finally {
                db.commit();
            }

        }
    }

    public static BasicBSONList getGlobTransIDInDataGroup( Sequoiadb db,
            List< String > groupNames ) {
        BasicBSONList globTransIDGroups = new BasicBSONList();
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_SYSTEM,
                "{RawData:true,IsPrimary:true,GroupName: {'$ne':'SYSCatalogGroup'}}",
                "{'TransInfo.GlobLowTran':'','TransInfo.GlobExpireTran':'',GroupName:''}",
                "{GroupName:1}" );
        while ( cursor.hasNext() ) {
            BSONObject record = ( BSONObject ) cursor.getNext();
            for ( int i = 0; i < groupNames.size(); i++ ) {
                if ( record.get( "GroupName" ) == groupNames.get( i ) ) {
                    BSONObject transInfo = ( BSONObject ) record
                            .get( "TransInfo" );
                    BSONObject tmp = new BasicBSONObject( "GlobLowTran",
                            transInfo.get( "GlobLowTran" ) );
                    tmp.put( "GlobExpireTran",
                            transInfo.get( "GlobExpireTran" ) );
                    tmp.put( "GroupName", record.get( "GroupName" ) );
                    globTransIDGroups.add( tmp );
                }
            }

        }
        cursor.close();
        return globTransIDGroups;
    }

    public static BasicBSONList getMaxRBSCLInDataGroup( Sequoiadb db,
            List< String > groupNames ) {
        BasicBSONList rbsCLNameInGroups = new BasicBSONList();
        for ( int i = 0; i < groupNames.size(); i++ ) {
            Sequoiadb nodeMaster = db.getReplicaGroup( groupNames.get( i ) )
                    .getMaster().connect();
            BSONObject findOption = new BasicBSONObject( "Name",
                    new BasicBSONObject( "$regex", "^SYSRBS" ) );
            BSONObject sortOption = new BasicBSONObject( "Name", 1 );
            String rbsCLName = ( String ) nodeMaster
                    .getList( Sequoiadb.SDB_LIST_COLLECTIONS, findOption, null,
                            sortOption, null, 0, 1 )
                    .getNext().get( "Name" );
            BSONObject rbsCLNameInGroup = new BasicBSONObject( "GroupName",
                    groupNames.get( i ) );
            rbsCLNameInGroup.put( "RbsCLName", rbsCLName );
            rbsCLNameInGroups.add( rbsCLNameInGroup );
        }
        return rbsCLNameInGroups;
    }

    public static void checkGlobTransIDInDataGroup( BasicBSONList expectList,
            BasicBSONList actualList ) {
        for ( int i = 0; i < actualList.size(); i++ ) {
            BSONObject lastGlobTransIDGroup = ( BSONObject ) actualList
                    .get( i );
            BSONObject globTransIDGroup = ( BSONObject ) expectList.get( i );
            if ( Long.parseLong(
                    ( ( String ) globTransIDGroup.get( "GlobLowTran" ) )
                            .substring( 2 ),
                    16 ) > Long.parseLong(
                            ( ( String ) lastGlobTransIDGroup
                                    .get( "GlobLowTran" ) ).substring( 2 ),
                            16 ) ) {
                throw new BaseException( -1000,
                        "GlobLowTran is Error in GroupName:"
                                + globTransIDGroup.get( "GroupName" )
                                + ", old GlobLowTran:"
                                + globTransIDGroup.get( "GlobLowTran" )
                                + "new GlobLowTran:"
                                + lastGlobTransIDGroup.get( "GlobLowTran" ) );
            }

        }
    }

    public static void checkRBSCLNameInGroups( BasicBSONList expectList,
            BasicBSONList actualList ) {
        for ( int i = 0; i < actualList.size(); i++ ) {
            BSONObject lastRBSCLNameInGroup = ( BSONObject ) actualList
                    .get( i );
            for ( int j = 0; j < expectList.size(); j++ ) {
                BSONObject rbsCLNameInGroup = ( BSONObject ) expectList
                        .get( j );
                if ( lastRBSCLNameInGroup.get( "GroupName" ) == rbsCLNameInGroup
                        .get( "GroupName" ) ) {

                    String rbsCLName = ( ( String ) rbsCLNameInGroup
                            .get( "RbsCLName" ) ).split( "\\." )[ 1 ];
                    String lastRBSCLName = ( ( String ) lastRBSCLNameInGroup
                            .get( "RbsCLName" ) ).split( "\\." )[ 1 ];
                    if ( rbsCLName.compareTo( lastRBSCLName ) >= 0 ) {
                        throw new BaseException( -1000,
                                "rbs cl check error in groupName:"
                                        + lastRBSCLNameInGroup
                                                .get( "GroupName" )
                                        + ", old rbs cl Name is: " + rbsCLName
                                        + ", new rbs cl Name: "
                                        + lastRBSCLName );
                    }

                }
            }
        }
    }

}
