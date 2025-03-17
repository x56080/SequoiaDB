package com.sequoiadb.recyclerecord;

import java.util.*;

import com.sequoiadb.recyclerecord.DeletingChecker;
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
 * @description seqDB-34292:事务删除与写操作并发
 * @author linsuqiang
 * @date 2025.3.12
 * @version 1.0
 */

public class RecycleRecord001 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String clName = "cl_34292";
    private int recordNum = 3000;
    private int writeTimeSec = 90;
    private int readTimeSec = 120;

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
        es.addWorker( new UpdateThd() );
        es.addWorker( new QueryThd() );
        es.addWorker( new CheckThd() );
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

        DeleteThd() {}

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
                    new Date() + " " + this.getClass().getName().toString() + " start" );
            try {
                for (int i = 0; i < recordNum; i++) {
                    db.beginTransaction();
                    dbcl.deleteRecords( new BasicBSONObject( "a", i ) );
                    db.commit();
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

    private class UpdateThd extends ResultStore {
        private Sequoiadb db = null;
        private DBCollection dbcl = null;
        private int updateValue = recordNum + 1;

        UpdateThd() {}

        @ExecuteOrder(step = 1)
        private void getcl() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dbcl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            BSONObject obj = new BasicBSONObject();
            obj.put("a", updateValue);
            obj.put("b", updateValue);
            obj.put("c", updateValue);
            dbcl.insert( obj );
        }

        @ExecuteOrder(step = 2)
        private void test() throws Exception {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() + " start" );
            try {
                long startMs = System.currentTimeMillis();
                long currentMs = startMs;
                while ((currentMs - startMs) < writeTimeSec * 1000) {
                    db.beginTransaction();
                    BSONObject matcher = new BasicBSONObject();
                    matcher.put("a", updateValue);
                    dbcl.updateRecords( matcher, new BasicBSONObject( "$inc", new BasicBSONObject("a", 1)));
                    updateValue += 1;
                    db.commit();
                    currentMs = System.currentTimeMillis();
                }
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
                System.out.println(
                        new Date() + " " + this.getClass().getName().toString() + " end" );
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
                for (int i = 0; i < readTimeSec; i++) {
                    db.beginTransaction();
                    BSONObject obj = dbcl.queryOne();
                    db.commit();
                    Thread.sleep( 1000 );
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

    private class CheckThd extends ResultStore {
        private DeletingChecker checker = null;

        CheckThd() {}

        @ExecuteOrder(step = 1)
        private void setup() throws Exception {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            checker = new DeletingChecker();
            checker.init( SdbTestBase.coordUrl, SdbTestBase.csName, clName, SdbTestBase.rootPwd );
        }

        @ExecuteOrder(step = 2)
        private void test() throws Exception {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try {
                // no recycle when writing
                int recycleDelaySec = 60;
                Thread.sleep( recycleDelaySec * 1000 );
                System.out.println(
                        new Date() + " start checking records not recycled" );
                int deletingCount = 0;
                int checkInterval1 = 5;
                int checkTimes = ( writeTimeSec - recycleDelaySec ) / checkInterval1;
                for (int i = 0; i < checkTimes; i++) {
                    deletingCount = checker.getTotalDeletingRecord();
                    if (0 == deletingCount) {
                        throw new Exception( "Records were recycled while writing. " +
                                "expect: " + recordNum + " actual: " +  deletingCount);
                    }
                    Thread.sleep( checkInterval1 * 1000 );
                }
                // do recycle when writing is stopped
                System.out.println(
                        new Date() + " start checking records recycled" );
                int checkInterval2 = 1;
                for (int i = 0; i < readTimeSec; i++) {
                    deletingCount = checker.getTotalDeletingRecord();
                    if (deletingCount == 0) {
                        break;
                    }
                    Thread.sleep( checkInterval2 * 1000 );
                }
                if (deletingCount != 0) {
                    throw new Exception( "Records were not recycled. " +
                            "expect: " + 0 + " actual: " +  deletingCount);
                }

            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                checker.fini();
            }
        }
    }
}
