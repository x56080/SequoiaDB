package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:Transaction16304 test query() with QUERY_FLG_FOR_UPDATE
 * @author wangkexin
 * @Date 2018-10-29
 * @version 1.00
 */

@Test(groups = "rc")
public class Transaction16304 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb sdb2;
    private CollectionSpace cs;
    private DBCollection cl;
    private DBCollection cl1;
    private DBCollection cl2;
    private String clName = "cl16304";
    private String commCSName;
    private ArrayList< BSONObject > insertRecods;

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        commCSName = SdbTestBase.csName;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            sdb2 = new Sequoiadb( coordAddr, "", "" );
            cs = sdb.getCollectionSpace( commCSName );
            cl = cs.createCollection( clName,
                    new BasicBSONObject( "ReplSize", 0 ) );
            insertData();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestTransaction16304 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    @Test
    private void testTrans16304() {
        // 因query接口使用QUERY_FLG_FOR_UPDATE执行查询已在用例17111中覆盖，这里只测试queryone接口
        sdb.beginTransaction();
        sdb2.beginTransaction();
        cl1 = sdb.getCollectionSpace( commCSName ).getCollection( clName );
        cl2 = sdb2.getCollectionSpace( commCSName ).getCollection( clName );

        BSONObject obj = cl1.queryOne( null, null, null, null,
                DBQuery.FLG_QUERY_FOR_UPDATE );
        BSONObject expobj = new BasicBSONObject();
        expobj.put( "_id", 0 );
        expobj.put( "num", 0 );
        Assert.assertEquals( obj, expobj );

        CL2Update cl2Update = new CL2Update();
        cl2Update.start();
        Assert.assertTrue( cl2Update
                .matchBlockingMethod( cl2.getClass().getName(), "update" ) );

        sdb.commit();
        Assert.assertTrue( cl2Update.isSuccess(), cl2Update.getErrorMsg() );
        sdb2.commit();
        checkResultAfterUpdate( cl2 );
    }

    @AfterClass
    public void tearDown() {
        sdb.commit();
        sdb2.commit();
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.close();
            sdb2.close();
        }
    }

    private void insertData() {
        try {
            BSONObject bson;
            insertRecods = new ArrayList< BSONObject >();
            for ( int i = 0; i < 10000; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                bson.put( "num", i );
                insertRecods.add( bson );
            }
            cl.insert( insertRecods, 0 );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestTransaction16304 insertData error, error description:"
                            + e.getMessage() );
        }
    }

    private void checkResultAfterUpdate( DBCollection cl ) {
        List< BSONObject > actualList = new ArrayList< BSONObject >();
        DBCursor cursor = cl.query();
        while ( cursor.hasNext() ) {
            actualList.add( cursor.getNext() );
        }
        cursor.close();

        List< BSONObject > expectedList = new ArrayList< BSONObject >();
        for ( int i = 0; i < insertRecods.size(); i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj = insertRecods.get( i );
            obj.put( "num", 22 );
            expectedList.add( obj );
        }
        Assert.assertEquals( actualList, expectedList );
    }

    private class CL2Update extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            DBQuery query = new DBQuery();
            query.setModifier( ( BSONObject ) JSON.parse( "{$set:{num:22}}" ) );
            cl2.update( query );
        }
    }
}
