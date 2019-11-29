package com.sequoiadb.auth;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

public class Util {
    public static boolean isCluster( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            int errno = e.getErrorCode();
            if ( new BaseException( SDBError.SDB_RTN_COORD_ONLY )
                    .getErrorCode() == errno ) {
                System.out.println(
                        "This test is for cluster environment only." );
                return false;
            }
        }
        return true;
    }
}
