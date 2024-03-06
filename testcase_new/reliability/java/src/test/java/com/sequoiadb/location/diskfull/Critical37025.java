package com.sequoiadb.location.diskfull;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.commlib.*;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.location.LocationUtils;

/**
 * @Description seqDB-34025:节点启动Critical模式，强杀未启动Critical模式的节点
 * @Author liuli
 * @Date 2024.03.06
 * @UpdateAuthor liuli
 * @UpdateDate 2024.03.06
 * @version 1.10
 */
@Test(groups = "location")
public class Critical37025 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_37025";
    private String clName = "cl_37025";
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
        // 主节点启动Critical模式
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        String nodeName = group.getMaster().getNodeName();
        BasicBSONObject options = new BasicBSONObject();
        options.put( "MinKeepTime", 5 );
        options.put( "MaxKeepTime", 10 );
        options.put( "NodeName", nodeName );
        group.startCriticalMode( options );

        try {
            Thread.sleep( 10000 );
        } catch ( InterruptedException e ) {
            throw new RuntimeException( e );
        }
        
        // 插入数据过程中强杀两个备节点
        ArrayList< BasicBSONObject > slaveNodeNames = LocationUtils
                .getGroupSlaveNodes( sdb, groupName );
        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject slaveNodeName : slaveNodeNames ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    slaveNodeName.getString( "hostName" ),
                    slaveNodeName.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }
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
