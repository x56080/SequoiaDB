package com.sequoiadb.datasync.brokennetwork;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
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
 * FileName: CreateCLAndPrimaryNodeCutNet2933.java test content:when create cl,
 * the data group master node breaks the net testlink case:seqDB-2933
 * 
 * @author wuyan
 * @Date 2017.5.2
 * @version 1.00
 */

public class CreateCLAndPrimaryNodeCutNet2933 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private boolean clearFlag = false;
    private String preCLName = "cl_2933";
    private String clGroupName = null;
    private static final int CL_NUM = 500;
    private String connectUrl;
    private String brokenNetHost;
    private int count = 0;

    @BeforeClass
    public void setUp() {
        try {
            System.out.println( this.getClass().getName() + " begin at:"
                    + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss.SSS" )
                            .format( new Date() ) );

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            String cataPriHost = cataGroup.getMaster().hostName();
            brokenNetHost = groupMgr.getGroupByName( clGroupName ).getMaster()
                    .hostName();
            if ( cataPriHost.equals( brokenNetHost )
                    && !cataGroup.changePrimary() ) {
                throw new SkipException(
                        cataGroup.getGroupName() + " reelect fail" );
            }

            connectUrl = CommLib.getSafeCoordUrl( brokenNetHost );
            sdb = new Sequoiadb( connectUrl, "", "" );
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void test() {
        try {
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( brokenNetHost, 3, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            CreateCLTask dTask = new CreateCLTask();
            mgr.addTask( dTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal and lsn consistency ,the
            // longest waiting time is 600S
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );

            // check result
            checkCreateCLResult();

            // check whether the cluster is normal and lsn consistency ,the
            // longest waiting time is 600S
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );
            checkConsistency();
            // Normal operating environment
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( clearFlag ) {
                dropCL();
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            System.out.println( this.getClass().getName() + " end at:"
                    + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss.SSS" )
                            .format( new Date() ) );
        }
    }

    private class CreateCLTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( connectUrl, "", "" )) {
                CollectionSpace commCS = db
                        .getCollectionSpace( SdbTestBase.csName );
                for ( int i = 0; i < CL_NUM; i++ ) {
                    String clName = preCLName + "_" + i;
                    BSONObject option = ( BSONObject ) JSON
                            .parse( "{ ShardingKey: { a: 1 },"
                                    + "ShardingType: 'hash', "
                                    + "Partition: 2048, " + "ReplSize: 2, "
                                    + "Compressed: true, "
                                    + "CompressionType: 'lzw',"
                                    + "IsMainCL: false, " + "AutoSplit: false, "
                                    + "Group: '" + clGroupName + "', "
                                    + "AutoIndexId: true, "
                                    + "EnsureShardingIndex: true }" );
                    commCS.createCollection( clName, option );
                    count++;
                }
            } catch ( BaseException e ) {
                System.out.println( "the create cl error i is =" + count );
            }
        }
    }

    /**
     * check the result of create cl,create cl after recovery from failure
     */
    private void checkCreateCLResult() {
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ ShardingKey: { a: 1 }," + "ShardingType: 'hash', "
                        + "Partition: 2048, " + "ReplSize: 2, "
                        + "Compressed: true, " + "CompressionType: 'lzw',"
                        + "IsMainCL: false, " + "AutoSplit: false, "
                        + "Group: '" + clGroupName + "', "
                        + "AutoIndexId: true, "
                        + "EnsureShardingIndex: true }" );

        // when the last cl fails,which may have actually been created
        // successfully,may fail again
        try {
            cs.createCollection( preCLName + "_" + count, option );
        } catch ( BaseException e ) {
            // -22 SDB_DMS_EXIST
            if ( e.getErrorCode() != -22 ) {
                Assert.fail( "the error not -22: " + e.getErrorCode()
                        + e.getErrorType() );
            }
        }

        // create remaining cl
        try {
            int beginNo = count + 1;
            for ( int i = beginNo; i < CL_NUM; i++ ) {
                cs.createCollection( preCLName + "_" + i, option );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "create cl fail: " + e.getErrorCode() + e.getErrorType() );
        }

        // randomly select a cl insert data
        String clName = "";
        try {
            Random random = new Random();
            clName = preCLName + "_" + random.nextInt( CL_NUM );
            DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            cl.insert( "{a:1,b:2}" );
        } catch ( BaseException e ) {
            Assert.fail( clName + " insert fail: " + e.getErrorCode()
                    + e.getErrorType() );
        }
    }

    private void checkConsistency() {
        GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
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

    private void dropCL() {
        CollectionSpace commCS = sdb.getCollectionSpace( SdbTestBase.csName );
        for ( int i = 0; i < CL_NUM; i++ ) {
            String clName = preCLName + "_" + i;
            try {
                commCS.dropCollection( clName );
            } catch ( BaseException e ) {
                Assert.fail( "drop cl fail: " + e.getErrorCode()
                        + e.getErrorType() );
            }
        }
    }
}