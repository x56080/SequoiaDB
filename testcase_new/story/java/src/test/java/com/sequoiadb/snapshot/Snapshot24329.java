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
 * @Description: seqDB-24329:在存在事务锁等待的 data 、coord 节点上执行死锁检测
 * @Author Yang Qincheng
 * @Date 2021.08.31
 */
public class Snapshot24329 extends SdbTestBase {
    private Sequoiadb db;
    private Sequoiadb dataNode = null;
    private final String clName1 = "cl_24329_A";
    private final String clName2 = "cl_24329_B";
    private final String clName3 = "cl_24329_C";
    private final static AtomicInteger count = new AtomicInteger( 3 );
    private final static Object obj = new Object();

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
        UpdateTrans t1 = new UpdateTrans( clName1, clName2 );
        UpdateTrans t2 = new UpdateTrans( clName2, clName3 );
        UpdateTrans t3 = new UpdateTrans( clName3, "" );
        GetAndCheckSnap snap = new GetAndCheckSnap();

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
        private String clName1;
        private String clName2;
        private Sequoiadb db;
        private String modifier = "{$set: {a: 2}}";

        UpdateTrans( String clName1, String clName2 ) {
            this.clName1 = clName1;
            this.clName2 = clName2;
            this.db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        @Override
        public void exec() throws Exception {
            try {
                db.beginTransaction();
                setTransactionID( db );
                // phase 1: get lock
                DBCollection cl1 = db.getCollectionSpace( csName ).getCollection( clName1 );
                cl1.update( "", modifier, "" );
                count.decrementAndGet();
                while ( count.get() > 0 ) {
                    Thread.sleep( 100 );
                }
                // phase 2: trigger lock wait
                if ( !clName2.equals("") ){
                    DBCollection cl2 = db.getCollectionSpace( csName ).getCollection( clName2 );
                    cl2.update( "", modifier, "" );
                }else {
                    synchronized ( obj ){
                        obj.wait();
                    }
                }
                db.commit();
            }catch (BaseException e){
                e.printStackTrace();
            }finally {
                db.close();
            }
        }
    }

    class GetAndCheckSnap extends SdbThreadBase{

        @Override
        public void exec() throws Exception {
            if ( count.get() != 0 ){
                Thread.sleep( 100 );
            }
            // make sure all transaction in phase 2
            Thread.sleep( 1000 );
            try{
                // 1. coord
                checkResult( db );
                // 2. data
                if ( dataNode != null ){
                    checkResult( dataNode );
                }
            }finally {
                // notify all transactions to commit
                synchronized ( obj ){
                    obj.notifyAll();
                }
            }
        }

        private void checkResult( Sequoiadb db ){
            DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSDEADLOCK, "","","" );
            try{
                Assert.assertFalse( cursor.hasNext() );
            }finally {
                cursor.close();
            }
        }
    }
}
