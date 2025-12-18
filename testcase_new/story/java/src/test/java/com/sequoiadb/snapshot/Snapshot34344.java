package com.sequoiadb.snapshot;

import java.util.*;
import java.util.concurrent.locks.*;

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
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description: seqDB-34344:慢查询新增监控指标（SlowSyncInfo），检查正确性
 * @Author Lin Suqiang
 * @Date 2025.11.27
 */
public class Snapshot34344 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_34344";
    private String clName = "cl_34344";
    private String groupName = null;
    private int maxReplSize = 0;
    private String slaveHost = null;
    private int slavePort = 0;
    private boolean runSuccess = false;

    private boolean moreBreakPoint = true;
    private final ReentrantLock breakPointLock = new ReentrantLock();
    private final Condition breakPointCondition = breakPointLock.newCondition();
    private boolean hasBreakPointEvent = false;
    private int breakPointKeepMs = 0;

    private boolean moreSnapshot = true;
    private final ReentrantLock snapshotLock = new ReentrantLock();
    private final Condition snapshotCondition = snapshotLock.newCondition();
    private boolean hasSnapshotEvent = false;
    private BSONObject snapshotResult = null;
    private int snapshotDelayMs = 0;

    @BeforeClass
    public void setup(){
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        String currentName = null;
        for ( int i = 0; i < groupNames.size(); i++ ) {
            currentName = groupNames.get(i);
            List< BasicBSONObject > groupNodes =
                    CommLib.getGroupNodes( sdb, currentName );
            if ( groupNodes.size() > maxReplSize ) {
                groupName = currentName;
                maxReplSize = groupNodes.size();
            }
        }
        if (maxReplSize < 2) {
            throw new SkipException( "No any slave node" );
        }

        BSONObject options = new BasicBSONObject();
        options.put( "mongroupmask", "all:detail" );
        options.put( "monslowquerythreshold", 1 );
        options.put( "monslowsyncthreshold", 1 );
        sdb.updateConfig( options );

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName, (BSONObject)JSON.parse(
                "{ Group: '" + groupName + "', ReplSize: -1 }" ));

        System.out.println( groupName );
        ReplicaGroup rg = sdb.getReplicaGroup( groupName );
        Node slave = rg.getSlave();
        slaveHost = slave.getHostName();
        slavePort = slave.getPort();
        System.out.println( slaveHost + ":" + slavePort );
    }

    @Test
    public void test() throws Exception{
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new TestControlThread() );
        es.addWorker( new SnapshotThread() );
        es.addWorker( new BreakPointThread() );
        es.run();
        runSuccess = true;
    }

    private class SnapshotThread extends ResultStore {
        private Sequoiadb db = null;
        private Sequoiadb master = null;
        BSONObject condition = null;
        BSONObject sortBy = null;
        BSONObject hint = null;

        @ExecuteOrder(step = 1)
        private void getcl() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            condition = new BasicBSONObject( "Name", csName + "." + clName );
            sortBy = new BasicBSONObject( "StartTimestamp", -1 );
            BSONObject options = new BasicBSONObject( "viewHistory", false );
            hint = new BasicBSONObject( "$Options", options );
            master = db.getReplicaGroup( groupName ).getMaster().connect();
        }

        private void snapshot() throws InterruptedException {
            snapshotLock.lock();
            try {
                while (!hasSnapshotEvent) {
                    snapshotCondition.await();
                }
                Thread.sleep( snapshotDelayMs );
                DBCursor cursor = master.getSnapshot(
                        Sequoiadb.SDB_SNAP_QUERIES,
                        condition, new BasicBSONObject(), sortBy, hint,
                        0, 1 );
                // Assert.assertTrue( cursor.hasNext() );
                snapshotResult = null;
                while ( cursor.hasNext() ) {
                    snapshotResult = cursor.getNext();
                }
                cursor.close();
                hasSnapshotEvent = false;
                snapshotCondition.signal();
            } finally {
                snapshotLock.unlock();
            }
        }


        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try {
                while (moreSnapshot) {
                    snapshot();
                }
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } catch ( InterruptedException ie ) {
                ie.printStackTrace();
            } finally {
                db.close();
            }
        }
    }

    private class BreakPointThread extends ResultStore {
        private Sequoiadb db = null;

        @ExecuteOrder(step = 1)
        private void getcl() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        private void continueBreakPoint() throws InterruptedException {
            breakPointLock.lock();
            try {
                while (!hasBreakPointEvent) {
                    breakPointCondition.await();
                }
                Thread.sleep( breakPointKeepMs ); // make operation slow
                Sequoiadb.SptEvalResult evalResult = null;
                evalResult = db.evalJS("new Sdb('" + slaveHost + "', " + slavePort +
                        ").traceOff('/tmp/snapshot34344')");
                BSONObject errMsg = evalResult.getErrMsg();
                if ( errMsg != null ) {
                    System.out.println( "errMsg: " + errMsg.toString() );
                    Assert.assertTrue(false);
                }
                System.out.println( new Date() + " traceOff" );
                hasBreakPointEvent = false;
                breakPointCondition.signal();
            } finally {
                breakPointLock.unlock();
            }
        }

        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try {
                while (moreBreakPoint) {
                    continueBreakPoint();
                }
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } catch ( InterruptedException ie ) {
                ie.printStackTrace();
            } finally {
                db.close();
            }
        }
    }

    private class TestControlThread extends ResultStore {
        private DBCollection cl = null;

        @ExecuteOrder(step = 1)
        private void getcl() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            cl = sdb.getCollectionSpace( csName ).getCollection( clName );
        }

        private void makeSlowSync( int slowMs ) throws InterruptedException {
            breakPointLock.lock();
            try {
                while (hasBreakPointEvent) {
                    breakPointCondition.await();
                }
                Sequoiadb.SptEvalResult evalResult = null;
                evalResult = sdb.evalJS("new Sdb('" + slaveHost + "', " + slavePort +
                        ").traceOn(1024, new SdbTraceOption()." +
                        "breakPoints('_dmsStorageUnit::insertRecord'))");
                BSONObject errMsg = evalResult.getErrMsg();
                if ( errMsg != null ) {
                    System.out.println( "errMsg: " + errMsg.toString() );
                    Assert.assertTrue(false);
                }
                System.out.println( new Date() + " traceOn" );
                hasBreakPointEvent = true;
                breakPointKeepMs = slowMs;
                breakPointCondition.signal();
            } finally {
                breakPointLock.unlock();
            }
        }

        private void asyncSnapshot( int delayMs ) throws InterruptedException {
            snapshotLock.lock();
            try {
                while (hasSnapshotEvent) {
                    snapshotCondition.await();
                }
                hasSnapshotEvent = true;
                snapshotDelayMs = delayMs;
                snapshotCondition.signal();
            } finally {
                snapshotLock.unlock();
            }
        }

        private void checkSlowInfo( BSONObject result, boolean isProcessing ) {
            System.out.println( result.toString() );

            BSONObject slowInfo = null;
            slowInfo = (BSONObject) result.get( "SlowSyncInfo" );
            Assert.assertTrue( slowInfo != null );
            int actualReplSize = (int) slowInfo.get( "ReplSize" );
            Assert.assertEquals( actualReplSize, maxReplSize );
            int actualSlowSyncCount = (int) slowInfo.get( "SlowSyncCount" );
            Assert.assertEquals( actualSlowSyncCount, 1 );
            long waitLSN = (long) slowInfo.get( "WaitLSN" );
            List<BSONObject> details = (List<BSONObject>) slowInfo.get( "Details" );
            String breakPointNode = slaveHost + ":" + slavePort;

            for ( int i = 0; i < details.size(); i++ ) {
                BSONObject detail = details.get( i );
                System.out.println( "------------------" );
                System.out.println( detail.toString() );
                String nodeName = (String) detail.get( "NodeName" );
                double waitTime = (double) detail.get( "WaitTime" );
                boolean synced = (boolean) detail.get( "Synced" );

                BSONObject startPoint = (BSONObject) detail.get( "StartPoint" );
                long startDiffToWaitLSN = (long) startPoint.get( "DiffToWaitLSN" );
                long startCompleteLSN = (long) startPoint.get( "CompleteLSN" );
                long startSyncNextLSN = (long) startPoint.get( "SyncNextLSN" );

                // Assert.assertTrue( startDiffToWaitLSN > 0 );
                Assert.assertTrue(
                        ( startCompleteLSN + startDiffToWaitLSN ) == waitLSN );
                Assert.assertTrue( startSyncNextLSN >= startCompleteLSN );

                BSONObject endPoint = (BSONObject) detail.get( "EndPoint" );
                long endDiffToWaitLSN = (long) endPoint.get( "DiffToWaitLSN" );
                long endCompleteLSN = (long) endPoint.get( "CompleteLSN" );
                long endSyncNextLSN = (long) endPoint.get( "SyncNextLSN" );

                if (isProcessing && nodeName.equals( breakPointNode )) {
                    Assert.assertTrue( waitTime == 0.0 );
                    Assert.assertTrue( !synced );
                    Assert.assertTrue( endDiffToWaitLSN > 0 );
                    Assert.assertTrue(
                            ( endCompleteLSN + endDiffToWaitLSN ) == waitLSN );
                    Assert.assertTrue( endSyncNextLSN >= endCompleteLSN );
                } else {
                    Assert.assertTrue( waitTime > 0.0 );
                    Assert.assertTrue( synced );
                    Assert.assertTrue( endDiffToWaitLSN <= 0 );
                    Assert.assertTrue(
                            ( endCompleteLSN + endDiffToWaitLSN ) == waitLSN );
                    Assert.assertTrue( endSyncNextLSN >= endCompleteLSN );
                }
            }
            System.out.println( "------------------" );
        }

        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try {
                List< BSONObject > records = new ArrayList< BSONObject >();
                for ( int i = 0; i < 1000; ++i ) {
                    BSONObject record = new BasicBSONObject();
                    record.put( "a", 1 );
                    record.put( "b", i );
                    record.put( "c", i );
                    record.put( "d", i );
                    record.put( "e", i );
                    record.put( "f", i );
                    record.put( "g", i );
                    record.put( "h", i );
                    records.add( record );
                }

                Thread.sleep( 500 ); // wait BreakPointThread to release lock
                makeSlowSync( 2000 );
                asyncSnapshot( 1000 );
                cl.insert( records );
                Thread.sleep( 1000 ); // wait slow log written

                // check slow log
                BSONObject condition = new BasicBSONObject();
                condition.put( "Name", csName + "." + clName );
                BSONObject sortBy = new BasicBSONObject( "StartTimestamp", -1 );
                BSONObject empty = new BasicBSONObject();
                Node master = sdb.getReplicaGroup( groupName ).getMaster();
                DBCursor cursor = master.connect().getSnapshot(
                        Sequoiadb.SDB_SNAP_QUERIES,
                        condition, empty, sortBy, empty, 0, 1 );
                Assert.assertTrue( cursor.hasNext() );
                while ( cursor.hasNext() ) {
                    BSONObject result = cursor.getNext();
                    checkSlowInfo( result, false );
                }
                cursor.close();

                // check processing query
                Assert.assertTrue( snapshotResult != null );
                checkSlowInfo( snapshotResult, true );

                // test no SlowSyncInfo
                sdb.updateConfig(
                        new BasicBSONObject( "monslowsyncthreshold", 60 * 1000 ) );
                makeSlowSync( 2000 );
                asyncSnapshot( 1000 );
                moreBreakPoint = false;
                moreSnapshot = false;
                cl.insert( records );
                Thread.sleep( 1000 ); // wait slow log written

                // check slow log
                cursor = master.connect().getSnapshot(
                        Sequoiadb.SDB_SNAP_QUERIES,
                        condition, empty, sortBy, empty, 0, 1 );
                Assert.assertTrue( cursor.hasNext() );
                while ( cursor.hasNext() ) {
                    BSONObject result = cursor.getNext();
                    BSONObject slowInfo = (BSONObject) result.get( "SlowSyncInfo" );
                    Assert.assertTrue( slowInfo == null );
                }
                cursor.close();

                // check processing query
                Assert.assertTrue( snapshotResult != null );
                BSONObject slowInfo =
                        (BSONObject) snapshotResult.get( "SlowSyncInfo" );
                Assert.assertTrue( slowInfo == null );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } catch ( InterruptedException ie ) {
                ie.printStackTrace();
            }
        }
    }


    @AfterClass
    public void tearDown(){
        try {
            BSONObject options = new BasicBSONObject();
            options.put( "mongroupmask", 1 );
            options.put( "monslowquerythreshold", 1 );
            options.put( "monslowsyncthreshold", 1 );
            sdb.deleteConfig( options, new BasicBSONObject() );
            sdb.dropCollectionSpace( csName );
        } finally {
            sdb.close();
        }
    }
}
