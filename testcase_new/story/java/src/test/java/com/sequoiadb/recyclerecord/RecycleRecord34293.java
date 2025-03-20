package com.sequoiadb.recyclerecord;

import java.util.*;

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.Ssh;
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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-34293:事务锁升级删除和查询并发
 * @author linsuqiang
 * @date 2025.3.12
 * @version 1.0
 */

public class RecycleRecord34293 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String clName = "cl_34293";
    private int recordNum = 20000;
    private boolean recycledFinished = false;

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

        BSONObject config = new BasicBSONObject();
        config.put("recordrecycledelay", 1);
        config.put("syncinterval", 1);
        sdb.updateConfig(config);
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( 300000 );
        es.addWorker( new DeleteThd() );
        es.addWorker( new QueryThd() );
        es.run();

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            BSONObject config = new BasicBSONObject();
            config.put("recordrecycledelay", 1);
            config.put("syncinterval", 1);
            sdb.deleteConfig( config, new BasicBSONObject() );
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
        private DeletingChecker checker = null;

        DeleteThd() {}

        @ExecuteOrder(step = 1)
        private void getcl() throws Exception {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dbcl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            db.setSessionAttr( new BasicBSONObject( "TransMaxLockNum", 10000 ) );
            checker = new DeletingChecker();
            checker.init( SdbTestBase.coordUrl, SdbTestBase.csName, clName, SdbTestBase.rootPwd );
        }

        @ExecuteOrder(step = 2)
        private void test() throws Exception {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() + " start" );
            try {
                db.beginTransaction();
                dbcl.deleteRecords( new BasicBSONObject() );
                int deletingCount = 0;
                while (deletingCount == 0) {
                    deletingCount = checker.getTotalDeletingRecord();
                    System.out.println( deletingCount );
                }
                db.commit();
                while (deletingCount > 0) {
                    deletingCount = checker.getTotalDeletingRecord();
                    System.out.println( deletingCount );
                }
                recycledFinished = true;
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
                checker.fini();
                System.out.println(
                        new Date() + " " + this.getClass().getName().toString() + " end");
            }
        }
    }

    private class QueryThd extends ResultStore {
        private Sequoiadb db = null;
        private DBCollection dbcl = null;

        QueryThd() {}

        @ExecuteOrder(step = 1)
        private void getcl() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dbcl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
        }

        @ExecuteOrder(step = 2)
        private void test() throws Exception {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() + " start");
            try {
                Random rand = new Random();
                BSONObject empty = new BasicBSONObject();
                while (!recycledFinished) {
                    db.beginTransaction();
                    BSONObject cond = new BasicBSONObject("a",
                            new BasicBSONObject( "$lte", rand.nextInt(recordNum)));
                    BSONObject obj = dbcl.queryOne(cond, empty, empty, empty, 0);
                    db.commit();
                    Thread.sleep( rand.nextInt( 500 ) );
                }

            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
                System.out.println(
                        new Date() + " " + this.getClass().getName().toString() + " end");
            }
        }
    }
}
