package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description:  seqDB-24861:RR 隔离级别下，锁升级事务回滚后老版本丢失
 * @Author Yang Qincheng
 * @Date 2021.12.16
 */
@Test( groups = "lockEscalation" )
public class Transaction24861 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private final String clName = "cl_24861";

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        BSONObject sessionAttr = new BasicBSONObject();
        sessionAttr.put( "TransIsolation", 3 );
        db1.setSessionAttr( sessionAttr );
        db2.setSessionAttr( sessionAttr );
        db3.setSessionAttr( sessionAttr );

        db1.getCollectionSpace( csName ).createCollection( clName );
        LockEscalationUtil.insertData( db1, csName, clName, 20 );
    }

    @AfterClass()
    public void tearDown() {
        try {
            db1.getCollectionSpace( csName ).dropCollection( clName );
        } finally {
            db1.close();
            db2.close();
            db3.close();
        }
    }

    @Test
    public void test() {
        try {
            db1.beginTransaction();
            DBCollection cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            BSONObject data = new BasicBSONObject();
            data.put( "a", "t1" );
            cl1.insert( data );

            db2.beginTransaction();
            db1.commit();

            db3.beginTransaction();
            DBCollection cl3 = db3.getCollectionSpace( csName ).getCollection( clName );
            cl3.update( "", "{$set: {b: \"t3\"}}", "" );
            LockEscalationUtil.checkCLLockType( db3, LockEscalationUtil.LOCK_X );

            db3.rollback();

            try {
                LockEscalationUtil.queryData( db2, csName, clName, data );
                Assert.fail( "Global transaction is not available exception expected!" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_GLOB_TRANS_NOT_AVAILABLE.getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            db1.commit();
            db2.commit();
            db3.commit();
        }
    }
}
