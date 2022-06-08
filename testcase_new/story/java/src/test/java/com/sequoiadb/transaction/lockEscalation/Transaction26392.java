package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-26392:RR 隔离级别下，读事务与写事务有交集，锁升级后老版本丢失
 * @Author liuli
 * @Date 2022.04.18
 * @UpdateAuthor liuli
 * @UpdateDate 2022.04.18
 * @version 1.10
 */
@Test(groups = "lockEscalation")
public class Transaction26392 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private final String clName = "cl_26392";
    private CollectionSpace dbcs = null;

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        BSONObject sessionAttr = new BasicBSONObject();
        sessionAttr.put( "TransIsolation", 3 );
        db1.setSessionAttr( sessionAttr );
        db2.setSessionAttr( sessionAttr );

        dbcs = db1.getCollectionSpace( csName );
        if ( dbcs.isCollectionExist( clName ) ) {
            dbcs.dropCollection( clName );
        }
        dbcs.createCollection( clName );
        LockEscalationUtil.insertData( db1, csName, clName, 20 );
    }

    @Test
    public void test() {
        db2.beginTransaction();
        db1.beginTransaction();

        LockEscalationUtil.queryData( db1, csName, clName, 0, 1 );

        try {
            DBCollection dbcl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            dbcl2.update( null, new BasicBSONObject( "$inc",
                    new BasicBSONObject( "b", 1 ) ), null );

        } finally {
            db2.commit();
        }

        try {
            LockEscalationUtil.queryData( db1, csName, clName, 15, 5 );
            Assert.fail(
                    "Global transaction is not available exception expected!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_GLOB_TRANS_NOT_AVAILABLE
                    .getErrorCode() ) {
                throw e;
            }
        } finally {
            db1.commit();
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            dbcs.dropCollection( clName );
        } finally {
            if ( db1 != null ) {
                db1.close();
            }
            if ( db2 != null ) {
                db2.close();
            }
        }
    }

}
