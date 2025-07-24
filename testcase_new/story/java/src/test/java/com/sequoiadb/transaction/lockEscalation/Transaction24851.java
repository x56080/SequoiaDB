package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-24851:CL 层面 S(DDL) 与 Z 的兼容性
 * @Author Yang Qincheng
 * @Date 2021.12.13
 */
<<<<<<< HEAD
@Test(groups = "lockEscalation")
=======
@Test( groups = "lockEscalation" )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
public class Transaction24851 extends SdbTestBase {
    private Sequoiadb db;
    private final String clName = "cl_24851";
    private final int cycleNum = 1000;
    private final static BSONObject EMPTY_BSON = new BasicBSONObject();

    @BeforeClass()
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db.getCollectionSpace( csName ).createCollection( clName );
    }

    @AfterClass()
    public void tearDown() {
        try {
            db.getCollectionSpace( csName ).dropCollection( clName );
        } finally {
            db.close();
        }
    }

    @Test
    public void test() {
        ZLockThread zLock = new ZLockThread();
        ULockThread uLock = new ULockThread();

        zLock.start();
        uLock.start();

        Assert.assertTrue( zLock.isSuccess(), zLock.getErrorMsg() );
        Assert.assertTrue( uLock.isSuccess(), uLock.getErrorMsg() );
    }

    class ULockThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
<<<<<<< HEAD
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
=======
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" ) ) {
                DBCollection cl = db.getCollectionSpace( csName ).getCollection( clName );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
                for ( int i = 0; i < cycleNum; i++ ) {
                    try {
                        cl.createIdIndex( EMPTY_BSON );
                        cl.dropIdIndex();
                    } catch ( BaseException e ) {
<<<<<<< HEAD
                        if ( e.getErrorCode() == SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode()
                                || e.getErrorCode() == SDBError.SDB_LOCK_FAILED
                                        .getErrorCode() ) {
                            break;
                        } else if ( e
                                .getErrorCode() == SDBError.SDB_DMS_TRUNCATED
                                        .getErrorCode() ) {
=======
                        if ( e.getErrorCode() == SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE.getErrorCode() ) {
                            break;
                        } else if ( e.getErrorCode() == SDBError.SDB_DMS_TRUNCATED.getErrorCode() ) {
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
                            continue;
                        }
                        throw e;
                    }
                }
            }
        }
    }

    class ZLockThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
<<<<<<< HEAD
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
=======
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" ) ) {
                DBCollection cl = db.getCollectionSpace( csName ).getCollection( clName );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
                for ( int i = 0; i < cycleNum; i++ ) {
                    try {
                        cl.truncate();
                    } catch ( BaseException e ) {
<<<<<<< HEAD
                        if ( e.getErrorCode() == SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode() ) {
                            break;
                        } else if ( e
                                .getErrorCode() == SDBError.SDB_CLS_COORD_NODE_CAT_VER_OLD
                                        .getErrorCode() ) {
=======
                        if ( e.getErrorCode() == SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE.getErrorCode() ) {
                            break;
                        } else if ( e.getErrorCode() == SDBError.SDB_CLS_COORD_NODE_CAT_VER_OLD.getErrorCode() ) {
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
                            continue;
                        }
                        throw e;
                    }
                }
            }
        }
    }
}