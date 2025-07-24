package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.CollectionSpace;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-26390:RR 隔离级别下，锁升级后同线程事务查询数据
 * @Author liuli
 * @Date 2022.04.18
 * @UpdateAuthor liuli
 * @UpdateDate 2022.04.18
 * @version 1.10
 */
@Test(groups = "lockEscalation")
public class Transaction26390 extends SdbTestBase {
    private Sequoiadb db;
    private DBCollection dbcl;
    private final String csName = "cs_26390";
    private final String clName = "cl_26390";
    private boolean runSuccess = false;
    private BasicBSONObject data = new BasicBSONObject();

    @BeforeClass()
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( db.isCollectionSpaceExist( csName ) ) {
            db.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = db.createCollectionSpace( csName );
        dbcl = dbcs.createCollection( clName );

        data.put( "a", 1 );
        dbcl.insert( data );

        BSONObject sessionAttr = new BasicBSONObject();
        sessionAttr.put( "TransIsolation", 3 );
        sessionAttr.put( "TransAutoCommit", true );
        sessionAttr.put( "TransMaxLockNum", 0 );
        db.setSessionAttr( sessionAttr );
    }

    @Test
    public void test() {
        data.put( "b", 1 );
        dbcl.update( null,
                new BasicBSONObject( "$inc", new BasicBSONObject( "b", 1 ) ),
                null );
        DBCursor cursor = dbcl.query();
        while ( cursor.hasNext() ) {
            Assert.assertEquals( cursor.getNext(), data );
        }
        cursor.close();
        runSuccess = true;
    }

    @AfterClass()
    public void tearDown() {
        try {
            if ( runSuccess ) {
                db.dropCollectionSpace( csName );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }
}
