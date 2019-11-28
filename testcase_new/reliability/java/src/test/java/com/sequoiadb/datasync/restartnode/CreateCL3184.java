package com.sequoiadb.datasync.restartnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
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
 * @FileName seqDB-3184: 创建CL过程中主节点节点正常重启，该主节点为同步的源节点
 * @Author linsuqiang
 * @Date 2017-03-29
 * @Version 1.00
 */

/*
 * 1.指定所有选项(Compressed、AutoIndexId)，批量创建CL 2.过程中 bin/sdbstop -p port &&
 * bin/sdbstart -c conf/local/port 3.选主成功后，继续创建 4.恢复 5.继续创建部分CL,查看CL信息
 * 6.随机选择一个CL，插入数据
 */

public class CreateCL3184 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clNameBase = "cl_3184";
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

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
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

            FaultMakeTask faultTask = NodeRestart.getFaultMakeTask( priNode, 1,
                    10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            CreateCLTask cTask = new CreateCLTask();
            mgr.addTask( cTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            checkConsistency( dataGroup );
            checkUsable( db );
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
            dropCLs( db );
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

    private class CreateCLTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                CollectionSpace commCS = db.getCollectionSpace( csName );
                for ( int i = 0; i < CL_NUM; i++ ) {
                    String clName = clNameBase + "_" + i;
                    BSONObject option = ( BSONObject ) JSON
                            .parse( "{ ShardingKey: { a: 1 },"
                                    + "ShardingType: 'hash', "
                                    + "Partition: 2048, " + "ReplSize: 1, "
                                    + "Compressed: true, "
                                    + "CompressionType: 'lzw',"
                                    + "IsMainCL: false, " + "AutoSplit: false, "
                                    + "Group: '" + clGroupName + "', "
                                    + "AutoIndexId: true, "
                                    + "EnsureShardingIndex: true }" );
                    commCS.createCollection( clName, option );
                }
            } catch ( BaseException e ) {
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

    private void checkUsable( Sequoiadb db ) {
        for ( int i = 0; i < CL_NUM; i++ ) {
            String clName = clNameBase + "_" + i;
            CollectionSpace commCS = db.getCollectionSpace( csName );
            // 有一个表可能创建失败
            if ( commCS.isCollectionExist( clName ) ) {
                DBCollection cl = commCS.getCollection( clName );
                cl.insert( "{ a: 1 }" );
            }
        }
    }

    private void dropCLs( Sequoiadb db ) {
        CollectionSpace commCS = db.getCollectionSpace( csName );
        for ( int i = 0; i < CL_NUM; i++ ) {
            String clName = clNameBase + "_" + i;
            try {
                commCS.dropCollection( clName );
            } catch ( BaseException e ) {
                // 有一个表可能创建失败了。
                if ( e.getErrorCode() != -23 ) {
                    throw e;
                }
            }
        }
    }
}