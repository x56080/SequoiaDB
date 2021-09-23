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
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description: seqDB-24352:死锁检测快照支持 sql 语法
 * @Author Yang Qincheng
 * @Date 2021.09.13
 */
public class Snapshot24352 extends SdbTestBase {
    private Sequoiadb db;
    private Sequoiadb dataNode = null;
    private final String clName1 = "cl_24352_A";
    private final String clName2 = "cl_24352_B";
    private final String clName3 = "cl_24352_C";
    private final static AtomicInteger count = new AtomicInteger( 3 );

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
            option1.put("Group", groupList.get( num ) );
            num = ++num < groupList.size() - 1 ? num : groupList.size() - 1;
            option2.put("Group", groupList.get( num ) );

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
        UpdateTrans t1 = new UpdateTrans(  "t1", clName1, clName2 );
        UpdateTrans t2 = new UpdateTrans(  "t2", clName2, clName3 );
        UpdateTrans t3 = new UpdateTrans(  "t3", clName3, clName2 );
        GetAndCheckSnap snap = new GetAndCheckSnap( t1, t2, t3 );

        t1.start();
        t2.start();
        t3.start();
        snap.start();

        t1.join();
        t2.join();
        t3.join();
        snap.join();

        for ( Throwable e : snap.getExceptions() ) {
            e.printStackTrace();
        }
        Assert.assertEquals( snap.getExceptions().size(), 0 );
    }

    class UpdateTrans extends SdbThreadBase {
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
            db.close();
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
        UpdateTrans trans1;
        UpdateTrans trans2;
        UpdateTrans trans3;

        GetAndCheckSnap(UpdateTrans trans1, UpdateTrans trans2, UpdateTrans trans3 ){
            this.trans1 = trans1;
            this.trans2 = trans2;
            this.trans3 = trans3;
        }

        @Override
        public void exec() throws Exception {
            if ( count.get() != 0 ){
                Thread.sleep( 100 );
            }
            // make sure all transaction in phase 2
            Thread.sleep( 1000 );
            try{

                // 1. SDB_SNAP_TRANSWAITS
                System.out.println( "get SDB_SNAP_TRANSWAITS info" );
                // coord
                getAndCheckTransDeadLock( db, 2, trans1, trans2, trans3 );
                // data
                if ( dataNode != null ){
                    getAndCheckTransDeadLock(dataNode, 2, trans1, trans2, trans3);
                }

                // 2. SDB_SNAP_TRANSWAITS
                System.out.println( "get SDB_SNAP_TRANSWAITS info" );
                // coord
                getAndCheckTransWait( db, 3,trans1, trans2, trans3 );
                // data
                if ( dataNode != null ){
                    getAndCheckTransWait(dataNode, 3,trans1, trans2, trans3);
                }

            }finally {
                // kill a transaction to unlock deadlocks
                System.out.println("kill " + trans2.name + " transaction to unlock deadlocks");
                trans2.interruptTrans();
            }
        }

        private void getAndCheckTransDeadLock(Sequoiadb db, int expectSize, UpdateTrans trans1, UpdateTrans trans2,
                                              UpdateTrans trans3 ) throws Exception{
            int count = 0;
            int deadlockID = -1;
            DBCursor cursor = db.exec("select * from $SNAPSHOT_TRANSDEADLOCK");
            try{
                while (cursor.hasNext()){
                    DeadlocksBean deadlock = new DeadlocksBean( cursor.getNext() );
                    Assert.assertTrue( deadlock.check() );
                    if (deadlockID == -1){
                        deadlockID = deadlock.getDeadlockID();
                    }else {
                        Assert.assertEquals( deadlock.getDeadlockID(), deadlockID);
                    }
                    String transID = deadlock.getTransID();
                    if ( !transID.equals( trans1.getTransactionID() ) &&
                         !transID.equals( trans2.getTransactionID() ) &&
                         !transID.equals( trans3.getTransactionID() ) ){
                        Assert.assertTrue( false,  "transaction id: " + transID + " do not match");
                    }
                    count++;
                }
                Assert.assertEquals(count, expectSize);
            }finally {
                cursor.close();
            }
        }


        private void getAndCheckTransWait(Sequoiadb db, int expectSize, UpdateTrans trans1, UpdateTrans trans2,
                                 UpdateTrans trans3 ) throws Exception{
            DBCursor cursor = db.exec("select * from $SNAPSHOT_TRANSWAIT");
            BSONObject result = null;
            int size = 0;
            try{
                while (cursor.hasNext()){
                    result = cursor.getNext();
                    String waiterTransID = (String)result.get("WaiterTransID");
                    String holderTransID = (String)result.get("HolderTransID");
                    // check value
                    if ( waiterTransID.equals( trans1.getTransactionID() )){
                        Assert.assertEquals(holderTransID, trans2.getTransactionID());
                    }
                    if ( waiterTransID.equals( trans2.getTransactionID() )){
                        Assert.assertEquals(holderTransID, trans3.getTransactionID());
                    }
                    if ( waiterTransID.equals( trans3.getTransactionID() )){
                        Assert.assertEquals(holderTransID, trans2.getTransactionID());
                    }
                    size++;
                }
                // check size
                Assert.assertEquals(size, expectSize);
                // check fields of snap
                Assert.assertNotNull(result.get("NodeName"));
                Assert.assertNotNull(result.get("GroupID"));
                Assert.assertNotNull(result.get("NodeID"));
                Assert.assertNotNull(result.get("WaitTime"));
                Assert.assertNotNull(result.get("WaiterTransID"));
                Assert.assertNotNull(result.get("HolderTransID"));
                Assert.assertNotNull(result.get("WaiterTransCost"));
                Assert.assertNotNull(result.get("HolderTransCost"));
                Assert.assertNotNull(result.get("WaiterSessionID"));
                Assert.assertNotNull(result.get("HolderSessionID"));
                Assert.assertNotNull(result.get("WaiterRelatedID"));
                Assert.assertNotNull(result.get("HolderRelatedID"));
                Assert.assertNotNull(result.get("WaiterRelatedSessionID"));
                Assert.assertNotNull(result.get("HolderRelatedSessionID"));
                Assert.assertNotNull(result.get("WaiterRelatedGroupID"));
                Assert.assertNotNull(result.get("HolderRelatedGroupID"));
                Assert.assertNotNull(result.get("WaiterRelatedNodeID"));
                Assert.assertNotNull(result.get("HolderRelatedNodeID"));
            }finally {
                cursor.close();
            }
        }
    }
}
