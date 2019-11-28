package com.sequoiadb.transaction;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-5999:多个事务并发，同时更新/删除cl中不同记录并提交事务_SD.transaction.010
 * @author wangkexin
 * @date 2019.03.15
 * @review
 */
public class Transaction5999 extends SdbTestBase {
    private String clName = "cl5999";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private BSONObject del_matcher = new BasicBSONObject();

    @BeforeClass
    public void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clName );
        insertData( cl );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        UpdateThread updateThread = new UpdateThread();
        DeleteThread deleteThread = new DeleteThread();

        threadExec.addWorker( updateThread );
        threadExec.addWorker( deleteThread );
        threadExec.run();

        int updateErrorCode = updateThread.getRetCode();
        int deleteErrorCode = deleteThread.getRetCode();
        if ( updateErrorCode == 0 && deleteErrorCode != 0 ) {
            if ( deleteErrorCode != -13 ) {
                Assert.fail( "delete thread fail:"
                        + deleteThread.getThrowable().getMessage() + "  e:"
                        + deleteErrorCode );
            }
            checkUpdateResult();
        } else if ( updateErrorCode != 0 && deleteErrorCode == 0 ) {
            if ( updateErrorCode != -13 ) {
                Assert.fail( "update thread fail:"
                        + updateThread.getThrowable().getMessage() + "  e:"
                        + updateErrorCode );
            }
            checkDeleteResult();
        } else if ( updateErrorCode == 0 && deleteErrorCode == 0 ) {
            checkUpdateResult();
            checkDeleteResult();
        } else {
            Assert.fail( "Unexpected results! updateThreadError:"
                    + updateThread.getThrowable().getMessage()
                    + "deleteThreadError:"
                    + deleteThread.getThrowable().getMessage() );
        }
    }

    @AfterClass
    public void teardown() {
        try {
            sdb.getCollectionSpace( csName ).dropCollection( clName );
        } finally {
            if ( sdb != null )
                sdb.close();
            if ( db1 != null )
                db1.close();
            if ( db2 != null )
                db2.close();
        }
    }

    private class UpdateThread extends ResultStore {
        private BSONObject matcher = new BasicBSONObject();
        private BSONObject modifyObj = new BasicBSONObject();
        private BSONObject modifier = new BasicBSONObject();
        private DBCollection cl1;

        private UpdateThread() {
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
        }

        @ExecuteOrder(step = 1)
        private void beginTrans() {
            db1.beginTransaction();
        }

        @ExecuteOrder(step = 3)
        private void update() throws BaseException {
            try {
                matcher.put( "b", 50 );
                modifyObj.put( "b", 5999 );
                modifier.put( "$set", modifyObj );
                cl1.update( matcher, modifier, null );
            } catch ( BaseException e ) {
                int errCode = e.getErrorCode();
                saveResult( errCode, e );
            }
        }

        @ExecuteOrder(step = 4)
        private void endTrans() {
            db1.commit();
        }
    }

    private class DeleteThread extends ResultStore {
        private DBCollection cl2;

        private DeleteThread() {
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
        }

        @ExecuteOrder(step = 2)
        private void beginTrans() {
            db2.beginTransaction();
        }

        @ExecuteOrder(step = 3)
        private void delete() {
            try {
                del_matcher.put( "a", 10 );
                cl2.delete( del_matcher );
            } catch ( BaseException e ) {
                int errCode = e.getErrorCode();
                saveResult( errCode, e );
            }
        }

        @ExecuteOrder(step = 5)
        private void endTrans() {
            db2.commit();
        }
    }

    private void checkUpdateResult() {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "b", 5999 );
        long actCount = cl.getCount( matcher );
        Assert.assertEquals( actCount, 1, "Update data does not exist!" );
    }

    private void checkDeleteResult() {
        long count = cl.getCount( del_matcher );
        Assert.assertEquals( count, 0, "Deleted data still exists!" );
    }

    private void insertData( DBCollection cl ) {
        List< BSONObject > recs = new ArrayList< BSONObject >();
        for ( int i = 0; i < 100; i++ ) {
            BSONObject rec = new BasicBSONObject();
            rec.put( "a", i );
            rec.put( "b", i );
            rec.put( "c", "string5999" );
            recs.add( rec );
        }
        cl.insert( recs );
    }
}
