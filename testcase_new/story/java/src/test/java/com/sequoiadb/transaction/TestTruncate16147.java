package com.sequoiadb.transaction;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.truncate.TruncateUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:seqDB-16147:数据组节点开启事务，并发执行非事务操作（bulkinsert和truncate并发） 插入数据，
 *                                                                  一条线程执行insert
 *                                                                  ，
 *                                                                  另一条线程执行truncate
 * @Author wangkexin
 * @Date 2018-09-18
 * @Version 1.00
 */
public class TestTruncate16147 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_16147";
    private boolean runSuccess = false;

    @BeforeClass(alwaysRun = true)
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            DBCollection cl = TruncateUtils.createCL( sdb, csName, clName );
            // doing insert
            insertDataBefore( cl );

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                CollectionSpace cs = sdb.getCollectionSpace( csName );
                if ( cs.isCollectionExist( clName ) ) {
                    cs.dropCollection( clName );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.close();
        }
    }

    @Test(groups = "ru")
    public void test() {
        TruncateThread truncateThread = new TruncateThread();
        InsertThread insertThread = new InsertThread();

        truncateThread.start();
        insertThread.start( 10 );

        if ( !( truncateThread.isSuccess() && insertThread.isSuccess() ) ) {
            Assert.fail(
                    truncateThread.getErrorMsg() + insertThread.getErrorMsg() );
        }
        runSuccess = true;
    }

    private class TruncateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                // doing truncate
                cl.truncate();
                cl.truncate();
            } catch ( BaseException e ) {
                e.printStackTrace();
                throw e;
            } finally {
                db.close();
            }
        }
    }

    private class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                // doing insert
                insertData( cl );
            } catch ( BaseException e ) {
                if ( -321 != e.getErrorCode() ) {
                    e.printStackTrace();
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.close();
            }
        }
    }

    private void insertDataBefore( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 100; i++ ) {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( int j = 0; j < 10000; j++ ) {
                int value = count++;
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + value
                        + ", test:" + "'testasetatatatatat'" + "}" );
                list.add( obj );
            }
            cl.insert( list );
        }
    }

    private void insertData( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 10; i++ ) {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( int j = 0; j < 10000; j++ ) {
                int value = count++;
                BSONObject obj = ( BSONObject ) JSON.parse(
                        "{sk:" + value + ", test:" + "'insert_test'" + "}" );
                list.add( obj );
            }
            cl.insert( list );
        }
    }
}
