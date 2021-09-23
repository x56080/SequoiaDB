package com.sequoiadb.snapshot;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description: seqDB-24330/seqDB-24331:在 data 节点上检测简单死锁，并使用 forceSession() 解除死锁
 * @Author Yang Qincheng
 * @Date 2021.09.02
 */
public class Snapshot24331 extends SdbTestBase {
    private Sequoiadb db;
    private Sequoiadb dataNode = null;
    private final String clName1 = "cl_24331_A";
    private final String clName2 = "cl_24331_B";
    private final String clName3 = "cl_24331_C";
    private final static AtomicInteger count = new AtomicInteger(3);

    @BeforeClass
    public void setUp(){
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        BSONObject option1 = new BasicBSONObject();
        BSONObject option2 = new BasicBSONObject();
        if ( !CommLib.isStandAlone( db ) ){
            ArrayList< String > groupList =  CommLib.getDataGroupNames( db );
            if ( groupList == null || groupList.size() < 1 ){
                throw new BaseException( SDBError.SDB_SYS, "The sequoiadb cluster is missing data groups" );
            }
            int num = 0;
            option1.put( "Group", groupList.get( num ) );
            num = ++num < groupList.size() - 1 ? num : groupList.size() - 1;
            option2.put( "Group", groupList.get( num ) );
            String nodeName = db.getReplicaGroup( groupList.get( num ) ).getMaster().getNodeName();
            dataNode = new Sequoiadb( nodeName, "", "" );
        }
        DBCollection cl1 = db.getCollectionSpace( csName ).createCollection( clName1, option1 );
        DBCollection cl2 = db.getCollectionSpace( csName ).createCollection( clName2, option2 );
        DBCollection cl3 = db.getCollectionSpace( csName ).createCollection( clName3, option2 );

        BSONObject doc = new BasicBSONObject( "a", 1 );
        cl1.insert( doc );
        cl2.insert( doc );
        cl3.insert( doc );
    }

    @AfterClass
    public void tearDown(){
        db.getCollectionSpace( csName ).dropCollection( clName1 );
        db.getCollectionSpace( csName ).dropCollection( clName2 );
        db.getCollectionSpace( csName ).dropCollection( clName3 );
        db.close();
        if ( dataNode != null ){
            dataNode.close();
        }
    }

    @Test
    public void test(){
        UpdateTrans t1 = new UpdateTrans( "t1", clName1, clName2 );
        UpdateTrans t2 = new UpdateTrans( "t2", clName2, clName3 );
        UpdateTrans t3 = new UpdateTrans( "t3", clName3, clName2 );
        List<UpdateTrans> transList = new ArrayList<>();
        transList.add( t2 );
        transList.add( t3 );
        GetAndCheckSnap snap = new GetAndCheckSnap( transList );

        t1.start();
        t2.start();
        t3.start();
        snap.start();

        t1.join();
        t2.join();
        t3.join();
        snap.join();

        for ( Throwable e: snap.getExceptions() ){
            e.printStackTrace();
        }
        Assert.assertEquals( snap.getExceptions().size(), 0 );
    }

    class UpdateTrans extends SdbThreadBase{
        private String name;
        private String clName1;
        private String clName2;
        private Sequoiadb db;
        private String modifier = "{$set: {a: 2}}";

        UpdateTrans( String name, String clName1, String clName2 ) {
            this.name = name;
            this.clName1 = clName1;
            this.clName2 = clName2;
            this.db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        void interruptTrans(){
            try {
                db.close();
            }catch (BaseException e){
                // do nothing
            }
        }

        @Override
        public void exec() throws Exception {
            try {
                DBCollection cl1 = db.getCollectionSpace( csName ).getCollection( clName1 );
                DBCollection cl2 = db.getCollectionSpace( csName ).getCollection( clName2 );

                db.beginTransaction();
                setTransactionID( db );
                // phase 1: get lock
                System.out.println( "thread " + name + " in phase 1: get lock" );
                cl1.update( "", modifier, "" );
                count.decrementAndGet();
                while ( count.get() > 0 ) {
                    Thread.sleep( 100 );
                }
                // phase 2: trigger lock wait
                System.out.println( "thread " + name + " in phase 2: trigger lock wait" );
                cl2.update( "", modifier, "" );
                db.commit();
            }catch (BaseException e){
                System.out.println( e.getMessage() );
            }finally {
                db.close();
            }
        }
    }

    class GetAndCheckSnap extends SdbThreadBase{
        private List<UpdateTrans> transList;

        GetAndCheckSnap( List<UpdateTrans> transList ){
            this.transList = transList;
        }

        @Override
        public void exec() throws Exception {
            if ( count.get() != 0 ){
                Thread.sleep( 100 );
            }
            // make sure all transaction in phase 2
            Thread.sleep( 1000 );

            System.out.println( "get SDB_SNAP_TRANSDEADLOCK info" );
            try {
                List<String> transIDList = new ArrayList<>();
                for ( UpdateTrans t: transList ){
                    transIDList.add( t.getTransactionID() );
                }
                long sessionID;

                // 1. get and check SDB_SNAP_TRANSDEADLOCK
                // coord
                sessionID = checkAndGetSessionID( db, transIDList, true, 2 );
                // data
                if ( dataNode != null ){
                    sessionID = checkAndGetSessionID( dataNode, transIDList, true, 2 );
                }

                // 2. use forceSession() unlock deadlocks
                if ( dataNode != null ){
                    // cluster mode
                    dataNode.forceSession( sessionID );
                    // wait for the transaction rollback to complete
                    Thread.sleep( 1000 );
                }else {
                    // standalone mode
                    db.forceSession( sessionID );
                    Thread.sleep( 1000 );
                }

                // 3. get and check SDB_SNAP_TRANSDEADLOCK
                checkAndGetSessionID( db, transIDList, false,  0 );
                // data
                if ( dataNode != null ){
                    checkAndGetSessionID( dataNode, transIDList, false, 0 );
                }
            }finally {
                // kill transactions to unlock deadlocks
                for ( UpdateTrans trans: transList ) {
                    trans.interruptTrans();
                }
            }
        }

        private long checkAndGetSessionID( Sequoiadb db, List<String> transIDList, boolean hadDeadlocks, int transNum ){
            int count = 0;
            int deadlockID = -1;
            long sessionID = -1;
            BSONObject matcher = new BasicBSONObject( "TransactionID", new BasicBSONObject( "$in", transIDList ) );
            DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSDEADLOCK, matcher,null,null );
            try{
                if ( hadDeadlocks ){
                    DeadlocksBean lastDeadLock = null;
                    while (cursor.hasNext()){
                        DeadlocksBean deadlock = new DeadlocksBean( cursor.getNext() );
                        Assert.assertTrue( deadlock.check() );
                        if ( deadlockID == -1 ){
                            deadlockID = deadlock.getDeadlockID();
                        }else {
                            // make sure all deadlockIDs are the same
                            Assert.assertEquals( deadlock.getDeadlockID(), deadlockID );
                        }
                        if ( lastDeadLock == null ){
                            lastDeadLock = deadlock;
                            sessionID = deadlock.getSession();
                        }else {
                            // make sure the results are orderly
                            Assert.assertTrue( lastDeadLock.compareTo(deadlock) );
                        }
                        count++;
                    }
                    Assert.assertEquals( count, transNum );
                }else {
                    Assert.assertFalse( cursor.hasNext() );
                }
                return sessionID;
            }finally {
                cursor.close();
            }
        }
    }
}
