package com.sequoiadb.transaction.rr;

import java.util.Date;
import java.util.List;
import java.util.Random;

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
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransRBS;
import com.sequoiadb.transaction.common.TransUtil;

/**
 * @Description seqDB-20465:事务操作过程中，catalog主节点正常停止120s后启动
 * @author zhaoyu
 * @date 2020-1-31
 *
 */
@Test(groups = "rrauto")
public class Transaction20465 extends SdbTestBase {
    private String clName = "transCL_20465";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private GroupMgr groupMgr;
    private int insertNum = 10000;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( csName );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }

        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( TransUtil.ClusterRestoreTimeOut ) ) {
            throw new SkipException( "GROUP ERROR" );
        }
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        BSONObject clOption = ( BSONObject ) JSON.parse(
                "{ShardingKey:{_id:1},ShardingType:'hash',AutoSplit:true}" );
        cl = cs.createCollection( clName, clOption );
        cl.createIndex( "a", "{a:1}", false, false );
        TransRBS.insertRandomLengthRecords( cl, insertNum, 400, 1024 );
        TransRBS.genMultiRBSCL( cl, TransRBS.loopNum );
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

        // 获取组上主节点的rbs最大集合id
        BasicBSONList rbsCLNameInGroups = TransRBS.getMaxRBSCLInDataGroup( sdb,
                groupNames );
        sdb.beginTransaction();

        // 正常重启catalog主节点
        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        NodeWrapper cataMaster = cataGroup.getMaster();

        // 建立并行任务
        FaultMakeTask faultTask = NodeRestart.getFaultMakeTask( cataMaster,
                new Random().nextInt( 10 ), 180 );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new TransUpdate() );
        mgr.execute();

        // TaskMgr检查线程异常
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待集群环境恢复
        Assert.assertTrue( groupMgr.checkBusinessWithLSN(
                TransUtil.ClusterRestoreTimeOut ), "GROUP ERROR" );

        // 事务查询，不报错
        cl = sdb.getCollectionSpace( csName ).getCollection( clName );
        DBCursor cursor = cl.query();
        TransUtil.getReadActList( cursor );

        // 继续产生老版本，使rbs集合切换
        TransRBS.genMultiRBSCL( cl, TransRBS.loopNum );

        // 比较组上的GlobLowTran及GlobExpireTran继续更新
        BasicBSONList lastGlobTransIDGroups = TransRBS
                .getGlobTransIDInDataGroup( sdb, groupNames );
        TransRBS.checkGlobTransIDInDataGroup( globTransIDGroups,
                lastGlobTransIDGroups );

        // 比较RBS集合一直在清理
        BasicBSONList lastRBSCLNameInGroups = TransRBS
                .getMaxRBSCLInDataGroup( sdb, groupNames );
        TransRBS.checkRBSCLNameInGroups( rbsCLNameInGroups,
                lastRBSCLNameInGroups );
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
