package com.sequoiadb.sdbschedule.utils;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.util.SdbDecrypt;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.ArrayList;
import java.util.List;

public class DataSourceUtils {
    private static volatile String coordUrls;
    private static volatile String user;
    private static volatile String cipherText;

    public static Sequoiadb getDsConnect( Sequoiadb sdb, String dsName ) {
        if ( coordUrls != null ) {
            return new Sequoiadb( getSdbUrlList( coordUrls ), user,
                    new SdbDecrypt().decryptPasswd( cipherText ),
                    new ConfigOptions() );
        }

        fetchDsConnectInfo( sdb, dsName );

        if ( coordUrls == null ) {
            throw new IllegalArgumentException(
                    "datasource info is incomplete, dsName=" + dsName );
        } else {
            return new Sequoiadb( getSdbUrlList( coordUrls ), user,
                    new SdbDecrypt().decryptPasswd( cipherText ),
                    new ConfigOptions() );
        }
    }

    private static synchronized void fetchDsConnectInfo( Sequoiadb sdb,
            String dsName ) {
        try ( DBCursor cursor = sdb.getList( Sequoiadb.SDB_LIST_DATASOURCES,
                new BasicBSONObject( "Name", dsName ), null, null, null, 0,
                1 )) {
            if ( !cursor.hasNext() ) {
                throw new IllegalArgumentException(
                        "datasource not found in SDB_LIST_DATASOURCES, dsName="
                                + dsName );
            }
            BSONObject next = cursor.getNext();
            cipherText = ( String ) next.get( "CipherText" );
            user = ( String ) next.get( "User" );
            coordUrls = ( String ) next.get( "Address" );
        }
    }

    public static List< String > getSdbUrlList( String sdbUrl ) {
        List< String > retList = new ArrayList<>();
        String[] urlArr = sdbUrl.split( "," );
        for ( String url : urlArr ) {
            retList.add( url );
        }
        return retList;
    }
}
