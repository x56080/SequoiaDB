package com.sequoiadb.location.diskfull;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-34026:Location启动Critical模式，强杀未启动Critical模式的节点
 * @Author liuli
 * @Date 2024.03.06
 * @UpdateAuthor liuli
 * @UpdateDate 2024.03.06
 * @version 1.10
 */
@Test(groups = "location")
public class Critical37026 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_37026";
    private String clName = "cl_37026";
    private String location1 = "location_37026_1";
    private String location2 = "location_37026_2";
    private String slaveNodeHostName;
    private String slaveNodeSvcName;

    private int recordNum = 100000;
    private String groupName;
    private List< BSONObject > batchRecords;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        groupName = groupMgr.getAllDataGroup().get( 0 ).getGroupName();

        // 主节点和一个备节点设置location1，另一个备节点设置location2
        ArrayList< BasicBSONObject > slaveNodeNames = LocationUtils
                .getGroupSlaveNodes( sdb, groupName );
        if ( slaveNodeNames.size() != 2 ) {
            throw new SkipException( "slave node number less than 2" );
        }
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        group.getMaster().setLocation( location1 );
        String slaveNodeName1 = slaveNodeNames.get( 0 ).getString( "hostName" )
                + ":" + slaveNodeNames.get( 0 ).getString( "svcName" );
        group.getNode( slaveNodeName1 ).setLocation( location1 );
        slaveNodeHostName = slaveNodeNames.get( 1 ).getString( "hostName" );
        slaveNodeSvcName = slaveNodeNames.get( 1 ).getString( "svcName" );
        group.getNode( slaveNodeHostName + ":" + slaveNodeSvcName )
                .setLocation( location2 );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "Group", groupName );
        options.put( "ReplSize", 0 );
        dbcl = dbcs.createCollection( clName, options );
    }

    @Test
    public void test() throws ReliabilityException {
        // 主节点所在location启动Critical模式
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        String nodeName = group.getMaster().getNodeName();
        BasicBSONObject options = new BasicBSONObject();
        options.put( "MinKeepTime", 5 );
        options.put( "MaxKeepTime", 10 );
        options.put( "Location", location1 );
        group.startCriticalMode( options );

        try {
            Thread.sleep( 10000 );
        } catch ( InterruptedException e ) {
            throw new RuntimeException( e );
        }

        // 插入数据过程中未启动Critical模式的备节点
        TaskMgr mgr = new TaskMgr();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( slaveNodeHostName,
                slaveNodeSvcName, 0 );
        mgr.addTask( faultTask );
        mgr.addTask( new Insert() );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 校验数据
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords, orderBy );

        group.start();
        group.stopCriticalMode();
        Assert.assertTrue( groupMgr.checkBusiness( 600 ),
                "failed to restore business" );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        LocationUtils.cleanLocation( sdb, groupName );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class Insert extends OperateTask {
        @Override
        public void exec() throws Exception {
            {
                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    DBCollection dbcl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    batchRecords = CommLib.insertData( dbcl, recordNum );
                }

            }
        }
    }

}
