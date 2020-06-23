package com.sequoiadb.transaction.rr;

import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransRBS;
import com.sequoiadb.transaction.common.TransUtil;

/**
 * @Description seqDB-20520:事务操作过程中，数据节点主节点正常停止120s后启动
 * @author zhaoyu
 * @date 2020-2-2
 *
 */
@Test(groups = "rrauto")
public class Transaction20520 extends SdbTestBase {
    private String clName = "cl20520";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private GroupMgr groupMgr;
    private CollectionSpace cs = null;
    private int insertNum = 10000;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }

        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( TransUtil.ClusterRestoreTimeOut ) ) {
            throw new SkipException( "GROUP ERROR" );
        }

        cs = sdb.getCollectionSpace( csName );
        BSONObject clOption = ( BSONObject ) JSON.parse(
                "{ShardingKey:{_id:1},ShardingType:'hash',AutoSplit:true}" );
        cl = cs.createCollection( clName, clOption );
        cl.createIndex( "a", "{a:1}", false, false );
        TransRBS.insertRandomLengthRecords( cl, insertNum, 400, 1024 );
        TransRBS.genMultiRBSCL( cl, 1 );
    }

    @AfterClass
    public void tearDown() throws InterruptedException {
        sdb.commit();
        cs.dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        // 获取组上的GlobLowTran及GlobExpireTran
        List< String > groupNames = groupMgr.getAllDataGroupName();
        BasicBSONList globTransIDGroups = TransRBS
                .getGlobTransIDInDataGroup( sdb, groupNames );

        sdb.beginTransaction();

        // 正常重启集合所在的数据节点主节点
        // 由于框架中,同一个组内获取的node,使用了同一个连接,通过下述方式使用不同连接停节点,避免使用同一个连接做并发停节点的操作,导致异常
        TaskMgr mgr = new TaskMgr();
        for ( int i = 0; i < groupNames.size(); i++ ) {
            groupMgr.setSdb( new Sequoiadb( SdbTestBase.coordUrl, "", "" ) );
            GroupWrapper dataGroup = groupMgr
                    .getGroupByName( groupNames.get( i ) );
            NodeWrapper dataMaster = dataGroup.getMaster();
            FaultMakeTask faultTask = NodeRestart.getFaultMakeTask( dataMaster,
                    60, 180 );
            mgr.addTask( faultTask );
        }

        // 建立并行任务
        mgr.addTask( new TransUpdate() );
        mgr.execute();

        // TaskMgr检查线程异常
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 最长等待10分钟的集群环境恢复
        Assert.assertTrue( groupMgr.checkBusinessWithLSN(
                TransUtil.ClusterRestoreTimeOut ), "GROUP ERROR" );

        // 事务查询报错
        cl = sdb.getCollectionSpace( csName ).getCollection( clName );
        try {
            cl.query();
            throw new BaseException( -1000, "Need throw error:"
                    + SDBError.SDB_GLOB_TRANS_NOT_AVAILABLE.getErrorCode() );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_GLOB_TRANS_NOT_AVAILABLE.getErrorCode() );
            sdb.rollback();
        }

        sdb.beginTransaction();
        DBCursor cursor = cl.query();
        TransUtil.getReadActList( cursor );

        // 继续产生老版本，使rbs集合切换
        TransRBS.genMultiRBSCL( cl, 1 );

        // 比较组上的GlobLowTran及GlobExpireTran继续更新
        BasicBSONList lastGlobTransIDGroups = TransRBS
                .getGlobTransIDInDataGroup( sdb, groupNames );
        TransRBS.checkGlobTransIDInDataGroup( globTransIDGroups,
                lastGlobTransIDGroups );

        // 比较数据节点的RBS集合重新从0号集合开始
        BasicBSONList rbsCLs = TransRBS.getMaxRBSCLInDataGroup( sdb,
                groupNames );
        for ( int i = 0; i < rbsCLs.size(); i++ ) {
            Assert.assertEquals(
                    ( ( BSONObject ) rbsCLs.get( i ) ).get( "RbsCLName" ),
                    "SYSRBS.SYSRBS0000" );
        }
    }

    class TransUpdate extends OperateTask {
        Sequoiadb db = null;

        @Override
        public void exec() throws Exception {
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                System.out.println( "trans update thread start:" + new Date() );
                for ( int i = 0; i < TransRBS.loopNum; i++ ) {
                    db.beginTransaction();
                    cl.update( null, "{$inc:{a:1}}", "{'':'a'}" );
                    db.commit();
                    cl.update( null, "{$inc:{a:1}}", "{'':'a'}" );
                }

            } catch ( BaseException e ) {
                e.printStackTrace();
            } finally {
                System.out.println( "trans update thread end:" + new Date() );
                db.close();
            }

        }
    }

}
