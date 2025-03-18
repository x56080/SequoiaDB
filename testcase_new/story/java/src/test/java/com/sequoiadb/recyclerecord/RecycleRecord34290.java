package com.sequoiadb.recyclerecord;

import java.util.*;

import com.sequoiadb.testcommon.CommLib;
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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-34290:事务删除与非事务删除并发
 * @description seqDB-34291:事务删除与事务查询并发
 * @author linsuqiang
 * @date 2025.3.12
 * @version 1.0
 */

public class RecycleRecord34290 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String clName = "cl_34290";
    private int recordNum = 6000;
    private int deleteThdCount = 0;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );

        DBCollection cl = cs.createCollection( clName );

        int batchNum = 1000;
        int count = 0;
        for ( int i = 0; i < recordNum / batchNum; i++ ) {
            List< BSONObject > batchRecords = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNum; j++ ) {
                BSONObject obj = new BasicBSONObject();
                obj.put("a", i * batchNum + j);
                obj.put("b", i * batchNum + j);
                obj.put("c", i * batchNum + j);
                batchRecords.add( obj );
            }
            cl.insert( batchRecords );
            batchRecords.clear();
        }
        cl.createIndex("aIdx", new BasicBSONObject("a", 1), false, false);

        sdb.updateConfig( new BasicBSONObject("recordrecycledelay", 1) );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new DeleteThd(deleteThdCount++, false) );
        es.addWorker( new DeleteThd(deleteThdCount++, true) );
        es.addWorker( new DeleteThd(deleteThdCount++, true) );
        es.addWorker( new QueryThd(0) );
        es.addWorker( new QueryThd(1) );
        es.addWorker( new QueryThd(2) );
        es.run();

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.deleteConfig( new BasicBSONObject("recordrecycledelay", 1),
                              new BasicBSONObject() );
            if ( runSuccess ) {
                if ( cs != null ) {
                    cs.dropCollection( clName );
                }
            }
        } finally {
            sdb.close();
        }
    }

    private class DeleteThd extends ResultStore {
        private Sequoiadb db = null;
        private DBCollection dbcl = null;
        private int seq = 0;
        private boolean useTrans = true;

        DeleteThd(int seq, boolean useTrans) {
            this.seq = seq;
            this.useTrans = useTrans;
        }

        @ExecuteOrder(step = 1)
        private void getcl() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dbcl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
        }

        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try {
                for (int i = seq; i < recordNum; i += deleteThdCount) {
                    if (useTrans) {
                        db.beginTransaction();
                    }
                    dbcl.deleteRecords( new BasicBSONObject( "a", i ) );
                    if (useTrans) {
                        db.commit();
                    }
                }
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class QueryThd extends ResultStore {
        private Sequoiadb db = null;
        private DBCollection dbcl = null;
        private int isolation = 0;

        QueryThd(int isolation) {
            this.isolation = isolation;
        }

        @ExecuteOrder(step = 1)
        private void getcl() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dbcl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            db.setSessionAttr(new BasicBSONObject("TransIsolation", isolation));
        }

        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try {
                Random rand = new Random();
                BSONObject empty = new BasicBSONObject();
                while (dbcl.getCount() > 0) {
                    db.beginTransaction();
                    BSONObject cond = new BasicBSONObject("a",
                            new BasicBSONObject( "$lte", rand.nextInt(recordNum)));
                    BSONObject obj = dbcl.queryOne(cond, empty, empty, empty, 0);
                    db.commit();
                }

            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }
}
