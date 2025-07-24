package com.sequoiadb.transaction.rr;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description seqDB-26372:index创建后对RR事务的睡眠机制验证
 * @author ZhangYanan
 * @createDate 2021.04.13
 * @updateUser ZhangYanan
 * @updateDate 2021.04.13
 * @updateRemark
 * @version v1.0
 */

@Test(groups = "rr")
public class Transaction26372 extends SdbTestBase {
    private Sequoiadb db = null;
    private String clName = "cl26372";
    private String IndexName = "aIndex_26372";
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() throws InterruptedException {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }

        cs = db.getCollectionSpace( csName );
        cl = cs.createCollection( clName );
        db.setSessionAttr(
                ( BSONObject ) JSON.parse( "{TransAutoCommit:true}" ) );
    }

    @Test
    public void test() {
        BSONObject insertData = new BasicBSONObject();
        insertData.put( "a", 1 );
        cl.insertRecord( insertData );
        cl.createIndex( IndexName, "{a:1}", true, false );
        cl.dropIndex( IndexName );
        cl.createIndex( IndexName, "{a:1}", true, false );
        BSONObject hint = new BasicBSONObject();
        hint.put( "", IndexName );
        DBCursor cursor = cl.query( "", "", "", hint.toString(),
                DBQuery.FLG_QUERY_FORCE_HINT );
        while ( cursor.hasNext() ) {
            BSONObject actData = cursor.getNext();
            actData.removeField( "_id" );
            Assert.assertEquals( insertData, actData );
        }
        cursor.close();
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            db.close();
        }
    }

}
