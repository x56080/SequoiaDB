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
 * @Description: seqDB-24325:在没有事务锁等待的 data、coord 节点上查询事务锁等待信息
 * @Author Yang Qincheng
 * @Date 2021.08.27
 */
public class Snapshot24325 extends SdbTestBase {
    private Sequoiadb db;
    private Sequoiadb dataNode = null;
    private final String clName = "cl_24325";
    private final Object syncObj = new Object();
    private final AtomicInteger count = new AtomicInteger(3);

    @BeforeClass
    public void setUp(){
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        DBCollection cl = db.getCollectionSpace( csName ).createCollection( clName );
        cl.insert( new BasicBSONObject( "a", 1 ) );

        if ( !CommLib.isStandAlone( db ) ){
            List< String > groupNameList =  CommLib.getCLGroups( cl );
            if ( groupNameList.size() < 1 ){
                throw new BaseException( SDBError.SDB_SYS, "The sequoiadb cluster is missing data groups" );
            }
            String nodeName = db.getReplicaGroup( groupNameList.get(0) ).getMaster().getNodeName();
            dataNode = new Sequoiadb( nodeName, "", "" );
        }
    }

    @AfterClass
    public void tearDown(){
        db.getCollectionSpace( csName ).dropCollection( clName );
        db.close();
        if ( dataNode != null ){
            dataNode.close();
        }
    }

    //@Test
    public void test(){
        QueryTrans t1 = new QueryTrans();
        QueryTrans t2 = new QueryTrans();
        QueryTrans t3 = new QueryTrans();
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

    class QueryTrans extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                DBCollection cl = db.getCollectionSpace( csName ).getCollection( clName );
                db.beginTransaction();
                cl.query();
                count.decrementAndGet();
                synchronized ( syncObj ){
                    syncObj.wait();
                }
                db.commit();
            }catch ( Exception e ){
                e.printStackTrace();
            }finally {
                db.close();
            }
        }
    }

    class GetAndCheckSnap extends SdbThreadBase{

        @Override
        public void exec() throws Exception {
            // all transactions should be in progress when the SDB_SNAP_TRANSWAITS is token
            if ( count.get() > 0 ){
                Thread.sleep( 100 );
            }
            try{

                // 1. coord
                checkResult( db );
                // 2. data
                if ( dataNode != null ){
                    checkResult( dataNode );
                }
            }finally {
                // notify all transactions to commit
                synchronized ( syncObj ){
                    syncObj.notifyAll();
                }
            }
        }

        private void checkResult( Sequoiadb db ){
            DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSWAITS, "","","" );
            try{
                Assert.assertFalse( cursor.hasNext() );
            }finally {
                cursor.close();
            }
        }
    }
}
