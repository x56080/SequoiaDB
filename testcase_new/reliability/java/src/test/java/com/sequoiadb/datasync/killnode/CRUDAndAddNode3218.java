package com.sequoiadb.datasync.killnode;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
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
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

/**
 * @FileName seqDB-3218: 文档写入加新建节点过程中主节点节点异常重启，该主节点为同步的源节点
 * @Author linsuqiang
 * @Date 2017-03-27
 * @Version 1.00
 */

/*
 * 1.创建CS，CL 2.循环执行增删改操作 3.往副本组中新增节点 4.过程中购造节点异常重启(kill -9) 5.选主成功后，继续写入
 * 6.过程中故障恢复 7.验证结果
 */

public class CRUDAndAddNode3218 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private TaskMgr taskMgr = null;
    private Sequoiadb db = null;
    private boolean runSuccess = false;
    private String clName = "cl_3218";
    private String clGroupName = null;
    private GroupWrapper clGroupWrapper = null;
    private SimpleDateFormat sdf = new SimpleDateFormat(
            "yyyy-MM-dd HH:mm:ss.S" );

    @BeforeClass
    public void setUp() {
        try {
            System.out.println( this.getClass().getName() + " begin at "
                    + sdf.format( new Date() ) );
            // 检测集群是否可用
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }
            // 准备用例的公用内容，如cl
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            Utils.makeReplicaLogFull( clGroupName );
            createCLOnGroup( db, clGroupName );
            clGroupWrapper = groupMgr.getGroupByName( clGroupName );
            NodeWrapper priNode = clGroupWrapper.getMaster();
            // 设置任务
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    priNode.hostName(), priNode.svcName(), 1 );
            taskMgr = new TaskMgr( faultTask );
            taskMgr.addTask( new CRUDTask() );
            taskMgr.addTask( new AddNodeTask() );
            // 各个任务各自初始化
            taskMgr.init();
        } catch ( BaseException | ReliabilityException e ) {
            if ( db != null ) {
                db.close();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void test() {
        try {
            // 启动任务
            taskMgr.start();
            taskMgr.join();

            if ( !groupMgr.checkBusiness( 600, true ) ) {
                Assert.fail( "checkBusinessWithExNode occurs time out(1)" );
            }
            // 各个任务检查各自结果
            // Note: 有包含过去的Assert.assertTrue(mgr.isAllSuccess(),
            // mgr.getErrorMsg());
            taskMgr.check();
            // 公共的结果检查，以下为检查cl所在数据组节点间一致性
            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithExNode occurs time out(2)" );
            }
            if ( !clGroupWrapper.checkInspect( 1 ) ) {
                Assert.fail( "data is different on "
                        + clGroupWrapper.getGroupName() );
            }
            runSuccess = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                // 各个任务分别清理自己环境
                taskMgr.fini();
                // 公用环境清理
                CollectionSpace commCS = db.getCollectionSpace( csName );
                commCS.dropCollection( clName );
            }
        } catch ( BaseException | ReliabilityException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
            System.out.println( this.getClass().getName() + " end at "
                    + sdf.format( new Date() ) );
        }
    }

    private DBCollection createCLOnGroup( Sequoiadb db, String clGroupName ) {
        CollectionSpace commCS = db.getCollectionSpace( csName );
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ Group: '" + clGroupName + "', ReplSize: 1 }" );
        return commCS.createCollection( clName, option );
    }

    private class CRUDTask extends OperateTask {
        private Sequoiadb db = null;
        private DBCollection cl = null;
        int insertedCnt = 0;

        @Override
        public void init() {
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.setSessionAttr( ( BSONObject ) JSON
                        .parse( "{ PreferedInstance: 'M' }" ) );
                cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
            } catch ( BaseException e ) {
                if ( db != null ) {
                    db.close();
                }
                throw e;
            }
        }

        @Override
        public void exec() throws Exception {
            try {
                int repeatTimes = 5000;
                for ( int i = 0; i < repeatTimes; i++ ) {
                    BSONObject rec = ( BSONObject ) JSON
                            .parse( "{ a: " + i + " }" );
                    cl.insert( rec );
                    BSONObject modifier = ( BSONObject ) JSON
                            .parse( "{ $set: { b: 1 } }" );
                    cl.update( rec, modifier, null );
                    cl.delete( rec );
                    cl.insert( rec );
                    insertedCnt++;
                }
            } catch ( BaseException e ) {
                // ignore
            }
        }

        @Override
        public void check() throws ReliabilityException {
            // 随机检查插入前数据
            // 副本数为1，丢数据为正常，检查插入前的数据无意义，因此注释
            // if (insertedCnt > 0) {
            // int randVal = new Random().nextInt(insertedCnt);
            // BSONObject randRec = (BSONObject) JSON.parse("{ a: " + randVal +
            // " }");
            // if (1 != cl.getCount(randRec)) {
            // throw new ReliabilityException("previous record " + randRec + "
            // not found");
            // }
            // }
            // 恢复后插入数据正常
            BSONObject rec = ( BSONObject ) JSON
                    .parse( "{ c: 'Hello World' }" );
            cl.insert( rec );
            if ( 1 != cl.getCount( rec ) ) {
                throw new ReliabilityException( "fail to insert into cl" );
            }
        }

        public void fini() {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private class AddNodeTask extends OperateTask {
        Sequoiadb db = null;
        private String randomHost = null;
        private int randomPort = 0;

        @Override
        public void init() {
            // 随机生成节点信息
            Random rand = new Random();
            List< String > hosts = groupMgr.getAllHosts();
            randomHost = hosts.get( rand.nextInt( hosts.size() ) );
            randomPort = rand.nextInt( reservedPortEnd - reservedPortBegin )
                    + reservedPortBegin;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                // 准备待同步的数据
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                insertData( cl );
                // 创建并启动新节点
                ReplicaGroup clGroup = db.getReplicaGroup( clGroupName );
                String nodePath = SdbTestBase.reservedDir + "/data/"
                        + randomPort;
                Node newNode = clGroup.createNode( randomHost, randomPort,
                        nodePath, ( BSONObject ) null );
                newNode.start();
            } catch ( BaseException e ) {
                if ( db != null ) {
                    db.close();
                }
                throw e;
            }
        }

        @Override
        public void exec() throws Exception {
            // 同步正在后台进行...
        }

        @Override
        public void fini() {
            try {
                removeNewNode( db );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }

        private void insertData( DBCollection cl ) {
            List< BSONObject > recs = new ArrayList< BSONObject >();
            int recNum = 100000;
            for ( int i = 0; i < recNum; i++ ) {
                BSONObject rec = ( BSONObject ) JSON.parse( "{ a: 1 }" );
                recs.add( rec );
            }
            cl.insert( recs, DBCollection.FLG_INSERT_CONTONDUP );
        }

        private void removeNewNode( Sequoiadb db ) {
            try {
                GroupWrapper clGroupWrapper = groupMgr
                        .getGroupByName( clGroupName );
                if ( clGroupWrapper.getMaster().svcName()
                        .equals( "" + randomPort ) ) {
                    clGroupWrapper.changePrimary();
                }
            } catch ( ReliabilityException e ) {
                e.printStackTrace();
            }
            ReplicaGroup clGroup = db.getReplicaGroup( clGroupName );
            clGroup.removeNode( randomHost, randomPort, ( BSONObject ) null );
        }
    }
}