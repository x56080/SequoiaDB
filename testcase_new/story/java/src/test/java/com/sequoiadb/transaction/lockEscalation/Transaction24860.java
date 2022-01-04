package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-24860:RR 隔离级别下，锁升级后老版本丢失
 * @Author Yang Qincheng
 * @Date 2021.12.16
 */
@Test(groups = "lockEscalation")
public class Transaction24860 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private final String clName = "cl_24860";

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        BSONObject sessionAttr = new BasicBSONObject();
        sessionAttr.put( "TransIsolation", 3 );
        db1.setSessionAttr( sessionAttr );
        db2.setSessionAttr( sessionAttr );

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
        }
    }

    @Test
    public void test(){
        db1.beginTransaction();
        db2.beginTransaction();

        try {
            DBCollection cl = db1.getCollectionSpace( csName ).getCollection( clName );
            cl.update(  "", "{$set: {b: 1}}", "");
            LockEscalationUtil.checkCLLockType( db1, LockEscalationUtil.LOCK_X );
        }finally {
            db1.commit();
        }

        try {
            LockEscalationUtil.queryData( db2, csName, clName, 0, 1 );
            Assert.fail( "Global transaction is not available exception expected!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_GLOB_TRANS_NOT_AVAILABLE.getErrorCode() ) {
                throw e;
            }
        }finally {
            db2.commit();
        }
    }
}
