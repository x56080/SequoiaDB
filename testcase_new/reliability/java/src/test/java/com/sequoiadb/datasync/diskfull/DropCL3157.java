package com.sequoiadb.datasync.diskfull;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.*;

/**
 * @FileName seqDB-3157: 删除CL过程中主节点磁盘满，该主节点为同步的源节点
 * @Author linsuqiang
 * @Date 2017-03-27
 * @Version 1.00
 */

/*
 * 1.批量删除CL 2.过程中构造磁盘满(dd购造) 3.继续删除 4.过程中故障恢复 5.验证结果
 */

public class DropCL3157 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clNameBase = "cl_3157";
    private String clGroupName = null;
    private static final int CL_NUM = 500;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {
            System.out.println( "the TestCase Name:" + this.getClass().getName()
                    + ". the TestCase begin at:"
                    + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss.SSS" )
                            .format( new Date() ) );

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            createCLs( db );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            NodeWrapper priNode = dataGroup.getMaster();

            FaultMakeTask faultTask = DiskFull.getFaultMakeTask(
                    priNode.hostName(), priNode.dbPath(), 0, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            DropCLTask dTask = new DropCLTask();
            mgr.addTask( dTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            checkConsistency( dataGroup );
            runSuccess = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( !runSuccess ) {
            throw new SkipException( "to save environment" );
        }
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dropRemainCLs( db );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
            System.out.println( "the TestCase Name:" + this.getClass().getName()
                    + ". the TestCase end at:"
                    + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss.SSS" )
                            .format( new Date() ) );
        }
    }

    private void createCLs( Sequoiadb db ) {
        CollectionSpace commCS = db.getCollectionSpace( csName );
        for ( int i = 0; i < CL_NUM; i++ ) {
            String clName = clNameBase + "_" + i;
            BSONObject option = ( BSONObject ) JSON
                    .parse( "{ Group: '" + clGroupName + "', ReplSize: 2 }" );
            commCS.createCollection( clName, option );
        }
    }

    private class DropCLTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace commCS = db.getCollectionSpace( csName );
                for ( int i = 0; i < CL_NUM; i++ ) {
                    String clName = clNameBase + "_" + i;
                    commCS.dropCollection( clName );
                }
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void checkConsistency( GroupWrapper dataGroup ) {
        List< String > dataUrls = dataGroup.getAllUrls();
        List< List< BSONObject > > results = new ArrayList< List< BSONObject > >();
        for ( String dataUrl : dataUrls ) {
            Sequoiadb dataDB = new Sequoiadb( dataUrl, "", "" );
            DBCursor cursor = dataDB.listCollections();
            List< BSONObject > result = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                result.add( cursor.getNext() );
            }
            results.add( result );
            cursor.close();
            dataDB.close();
        }

        List< BSONObject > compareA = results.get( 0 );
        sortByName( compareA );
        for ( int i = 1; i < results.size(); i++ ) {
            List< BSONObject > compareB = results.get( i );
            sortByName( compareB );
            if ( !compareA.equals( compareB ) ) {
                System.out.println( dataUrls.get( 0 ) );
                System.out.println( compareA );
                System.out.println( dataUrls.get( i ) );
                System.out.println( compareB );
                Assert.fail( "data is different. see the detail in console" );
            }
        }
    }

    private void sortByName( List< BSONObject > list ) {
        Collections.sort( list, new Comparator< BSONObject >() {
            public int compare( BSONObject a, BSONObject b ) {
                String aName = ( String ) a.get( "Name" );
                String bName = ( String ) b.get( "Name" );
                return aName.compareTo( bName );
            }
        } );
    }

    private void dropRemainCLs( Sequoiadb db ) {
        CollectionSpace commCS = db.getCollectionSpace( csName );
        for ( int i = 0; i < CL_NUM; i++ ) {
            String clName = clNameBase + "_" + i;
            try {
                commCS.dropCollection( clName );
            } catch ( BaseException e ) {
                // -23 SDB_DMS_NOTEXIST 集合不存在
                if ( !( e.getErrorCode() == -23 ) ) {
                    throw e;
                }
            }
        }
    }
}