package com.sequoiadb.datasource;

import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import com.sequoiadb.testcommon.*;

public class DataSourceTestBase extends SdbTestBase {
    protected String coordAddr;
    protected String userName = "";
    protected String password = "";
    protected String clName;
    protected static final int catalogGroupID = 2;
    protected ArrayList< InetSocketAddress > addrList = new ArrayList< InetSocketAddress >();
    private AtomicBoolean init = new AtomicBoolean( false );
    private Boolean isStandAlone = false;

    private String[] splitStr( String srcStr, String strSep ) {
        String[] tmpArr = srcStr.split( strSep );
        return tmpArr;
    }

    protected boolean isStandAlone() {
        return isStandAlone;
    }

    protected boolean init() {
        if ( init.get() )
            return true;
        this.coordAddr = SdbTestBase.coordUrl;
        if ( this.coordAddr == null ) {
            this.coordAddr = "localhost:11810";
        } else {
            /*
             * String[] tmpArr = splitStr(coordUrl, "@"); if (tmpArr.length ==
             * 2){ this.coordAddr = tmpArr[1]; userName = tmpArr[0]; tmpArr =
             * splitStr(userName, ":"); if (tmpArr.length == 2){ userName =
             * tmpArr[0]; password = tmpArr[1]; }else return false; }else{
             * this.coordAddr = tmpArr[0]; } tmpArr = splitStr(coordUrl, "/");
             * this.coordAddr = tmpArr[0]; if (tmpArr.length == 2){ csName =
             * tmpArr[1]; tmpArr = splitStr(csName, "\\."); csName = tmpArr[0];
             * if (tmpArr.length == 2){ clName = tmpArr[1]; }else{ return false;
             * } }
             */

            int index = this.coordAddr.indexOf( ':' );
            if ( -1 == index ) {
                this.coordAddr += ":11810";
            }
        }
        getAddrList();
        init.set( true );
        return true;
    }

    protected void judegeErrCode( String errType, int errCode ) {
        int expectErrCode = 0;
        try {
            expectErrCode = SDBError.valueOf( errType ).getErrorCode();
        } catch ( Exception e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        }

        Assert.assertEquals( errCode, expectErrCode );
    }

    protected void getAddrList() {
        try {
            if ( !addrList.isEmpty() )
                return;
            Sequoiadb sdb = new Sequoiadb( this.coordAddr, this.userName,
                    this.password );
            ReplicaGroup group = sdb.getReplicaGroup( catalogGroupID );
            BSONObject obj = group.getDetail();
            BasicBSONList subObjs = ( BasicBSONList ) obj.get( "Group" );

            ArrayList< InetSocketAddress > taddrList = new ArrayList< InetSocketAddress >();
            for ( int i = 0; i < subObjs.size(); ++i ) {
                BasicBSONObject doc = ( BasicBSONObject ) subObjs.get( i );
                String hostName = doc.getString( "HostName" );
                BasicBSONList serviceDocs = ( BasicBSONList ) doc
                        .get( "Service" );
                BasicBSONObject serviceDoc = ( BasicBSONObject ) serviceDocs
                        .get( 0 );
                String serviceName = serviceDoc.getString( "Name" );

                taddrList.add( new InetSocketAddress( hostName,
                        Integer.parseInt( serviceName ) ) );
            }
            addrList = taddrList;
        } catch ( BaseException e ) {

            try {
                if ( e.getErrorCode() == SDBError.SDB_RTN_COORD_ONLY
                        .getErrorCode() ) {
                    isStandAlone = true;
                    return;
                }
            } catch ( Exception e1 ) {
                // TODO Auto-generated catch block
                e1.printStackTrace();
            }
            Assert.assertTrue( false, e.getMessage() );
        }
    }

    protected InetSocketAddress getInCoordAddr() {
        String[] tmpArr = this.coordAddr.split( ":" );
        String hostName = tmpArr[ 0 ];
        int port = 11810;
        if ( tmpArr.length == 2 ) {
            port = Integer.parseInt( tmpArr[ 1 ] );
        }

        return new InetSocketAddress( hostName, port );
    }

    protected boolean isContainAddr( InetSocketAddress addr ) {
        if ( addrList.isEmpty() )
            return false;
        for ( int i = 0; i < addrList.size(); ++i ) {
            if ( addrList.get( i ).equals( addr ) ) {
                return true;
            }
        }
        return false;
    }
}
