package com.sequoiadb.crud.truncate;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:seqDB-176:upsert与truncate的并发 插入数据，一条线程执行upsert，另一条线程执行truncate
 * @Author linsuqiang
 * @Date 2016-12-06
 * @Version 1.00
 */
public class TestTruncate176 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_176";
    private SimpleDateFormat sdf = new SimpleDateFormat(
            "yyyy-MM-dd HH:mm:ss.S" );
    private BSONObject modifier = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        try {
            DBCollection cl = TruncateUtils.createCL( sdb, csName, clName );
            // doing insert
            TruncateUtils.insertData( cl );
            // prepare data for upsert
            modifier = new BasicBSONObject();
            BSONObject upsertValue = new BasicBSONObject();
            upsertValue.put( "newKey", "is upserted" );
            modifier.put( "$set", upsertValue );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.disconnect();
        }
    }

    @Test
    public void test() {
        TruncateThread truncateThread = new TruncateThread();
        UpsertThread upsertThread = new UpsertThread();

        upsertThread.start();
        truncateThread.start();

        if ( !( truncateThread.isSuccess() && upsertThread.isSuccess() ) ) {
            Assert.fail(
                    truncateThread.getErrorMsg() + upsertThread.getErrorMsg() );
        }
        // check result
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        if ( ( int ) cl.getCount() > 1 ) {
            Assert.fail( "truncate fail: data haven't been truncated clearly" );
        }
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
            } catch ( BaseException e ) {
                e.printStackTrace();
                throw e;
            } finally {
                db.disconnect();
            }
        }
    }

    private class UpsertThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                // doing upsert
                cl.upsert( null, modifier, null );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 ) {
                    throw e;
                }
            } finally {
                db.disconnect();
            }
        }
    }
}
