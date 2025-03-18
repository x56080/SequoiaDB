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
 * @description seqDB-34294:事务回滚和删除CS并发
 * @author linsuqiang
 * @date 2025.3.12
 * @version 1.0
 */

public class RecycleRecord34294 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String csName = "cs_34294";
    private String clName = "cl_34294";
    private int recordNum = 100000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        cs = sdb.createCollectionSpace( csName );

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
        BSONObject matcher = new BasicBSONObject(); 
        BSONObject rule = new BasicBSONObject("$set",
                new BasicBSONObject("a", "long_long_string_to_make_record_overflow"));
        cl.updateRecords( matcher, rule );
        cl.createIndex("aIdx", new BasicBSONObject("a", 1), false, false);
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( 300000 );
        es.addWorker( new DeleteThd() );
        es.addWorker( new DropCSThd() );
        es.run();

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( cs != null ) {
                    cs.dropCollection( clName );
                    sdb.dropCollectionSpace( csName );
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
            dbcl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            db.beginTransaction();
            dbcl.deleteRecords( new BasicBSONObject() );
        }

        @ExecuteOrder(step = 2)
        private void test() throws BaseException {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() + " start" );
            try {
                db.rollback();
            } finally {
                if ( db != null ) {
                    db.close();
                }
                System.out.println(
                        new Date() + " " + this.getClass().getName().toString() + " end");
            }
        }
    }

    private class DropCSThd extends ResultStore {
        private Sequoiadb db = null;
        private DBCollection dbcl = null;

        DropCSThd() {}

        @ExecuteOrder(step = 1)
        private void getcl() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        @ExecuteOrder(step = 2)
        private void test() throws Exception {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() + " start");
            try {
                db.dropCollectionSpace( csName );
            } catch (BaseException e) {
                if (e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE.getErrorCode()) {
                    throw e;
                }
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
