package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-24862:RR 隔离级别下，锁升级事务回滚后老版本未丢失
 * @Author Yang Qincheng
 * @Date 2021.12.16
 */
@Test( groups = "lockEscalation" )
public class Transaction24862 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private DBCollection cl;
    private final String clName = "cl_24862";

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        BSONObject sessionAttr = new BasicBSONObject();
        sessionAttr.put( "TransIsolation", 3 );
        db1.setSessionAttr( sessionAttr );
        db2.setSessionAttr( sessionAttr );

        cl = db1.getCollectionSpace( csName ).createCollection( clName );
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
    public void test() {
        BSONObject data = new BasicBSONObject();
        data.put( "a", "t1" );
        data.put( "b", "t1" );
        cl.insert( data );

        db1.beginTransaction();
        db2.beginTransaction();

        try {
            DBCollection cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl2.update( "", "{$set: {b: \"t2\"}}", "" );
            LockEscalationUtil.checkCLLockType( db2, LockEscalationUtil.LOCK_X );

            db2.rollback();

            DBCollection cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            BSONObject matcher = new BasicBSONObject("a", "t1");
            try (DBCursor cursor = cl1.query( matcher, null, null, null) ){
                Assert.assertEquals( cursor.getNext(), data );
            }
        }finally {
            db1.commit();
            db2.commit();
        }
    }
}
