package com.sequoiadb.faulttolerance.slownode;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.faulttolerance.FaultToleranceUtils;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22202
 *              容错级别为半容错，只有1个副本状态正常，其他副本状态为:SLOWNODE，replSize=-1的集合下插入数据
 * @author luweikang
 * @date 2020年6月5日
 */
public class Faulttolerance22202 extends SdbTestBase {

    private String csName = "cs22202";
    private String clName = "cl22202";
    private String testCLName = "cl22202_test";
    private byte[] lobBuff = LobUtil.getRandomBytes( 1024 * 1024 );
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private List< String > slaveNodeNames = new ArrayList<>();
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private boolean shutoff = false;

    @BeforeClass
    public void setUp() throws ReliabilityException {

        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }

        groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );

        cs.createCollection( testCLName, ( BSONObject ) JSON
                .parse( "{'Group': '" + groupName + "', ReplSize: -1}" ) );

        BSONObject config = new BasicBSONObject();
        config.put( "ftlevel", 2 );
        config.put( "ftmask", "SLOWNODE" );
        config.put( "ftfusingtimeout", 10 );
        config.put( "ftconfirmperiod", 3 );
        config.put( "ftslownodethreshold", 1 );
        config.put( "ftslownodeincrement", 1 );
        sdb.updateConfig( config,
                new BasicBSONObject( "GroupName", groupName ) );

        GroupWrapper gwp = groupMgr.getGroupByName( groupName );
        String masterName = sdb.getReplicaGroup( groupName ).getMaster()
                .getNodeName();
        List< String > allNodeName = gwp.getAllUrls();
        for ( String nodeName : allNodeName ) {
            if ( !nodeName.equals( masterName ) ) {
                slaveNodeNames.add( nodeName );

            }
        }
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {

        TaskMgr mgr = new TaskMgr();
        for ( int i = 0; i < 10; i++ ) {
            mgr.addTask( new Insert() );
            mgr.addTask( new Update() );
            mgr.addTask( new PutLobTask() );
        }
        mgr.addTask( new TestSlowNode() );

        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        BSONObject config = new BasicBSONObject();
        config.put( "ftlevel", 1 );
        config.put( "ftmask", 1 );
        config.put( "ftconfirmperiod", 1 );
        config.put( "ftslownodethreshold", 1 );
        config.put( "ftslownodeincrement", 1 );
        sdb.deleteConfig( config, new BasicBSONObject() );
        sdb.updateConfig( new BasicBSONObject( "ftfusingtimeout", 300 ) );

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }

        }
    }

    class Insert extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 1000; i++ ) {
                    if ( shutoff ) {
                        break;
                    }
                    ArrayList< BSONObject > records = new ArrayList<>();
                    for ( int j = 0; j < 5000; j++ ) {
                        BSONObject record = new BasicBSONObject();
                        record.put( "a", j );
                        record.put( "b", j );
                        record.put( "order", j );
                        record.put( "str",
                                "fjsldkfjlksdjflsdljfhjdshfjksdhfsdfhsdjkfhjkdshfj"
                                        + "kdshfkjdshfkjsdhfkjshafdkhasdikuhsdjfls"
                                        + "hsdjkfhjskdhfkjsdhfjkdshfjkdshfkjhsdjkf"
                                        + "hsdkjfhsdsafnweuhfuiwnqefiuokdjf" );
                        records.add( record );
                    }
                    cl.insert( records );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -105 && e.getErrorCode() != -252 ) {
                    throw e;
                }
            }
        }
    }

    class Update extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 1000; i++ ) {
                    if ( shutoff ) {
                        break;
                    }
                    cl.update( null,
                            "{$inc:{a:1, b:1}, $set:{'str':'update str times "
                                    + i + "'}}",
                            null );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -105 && e.getErrorCode() != -252 ) {
                    throw e;
                }
            }
        }
    }

    class PutLobTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 1000; i++ ) {
                    if ( shutoff ) {
                        break;
                    }
                    DBLob lob = cl.createLob();
                    lob.write( lobBuff );
                    lob.close();
                }
            }
        }
    }

    class TestSlowNode extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                for ( int i = 0; i < 6000; i++ ) {
                    boolean nodeSlow = true;
                    for ( String nodeName : slaveNodeNames ) {
                        String ft = FaultToleranceUtils.getNodeFTStatus( db,
                                nodeName );
                        if ( !ft.equals( "SLOWNODE" )
                                && !ft.equals( "SLOWNODE|DEADSYNC" ) ) {
                            nodeSlow = false;
                            break;
                        }
                    }
                    if ( nodeSlow ) {
                        break;
                    } else {
                        if ( i == 5999 ) {
                            shutoff = true;
                            System.out.println(
                                    "600 seconds still not executed." );
                            Assert.fail( "600 seconds still not executed." );
                        }
                        Thread.sleep( 100 );
                    }
                }

                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( testCLName );
                try {
                    cl.insert( "{a:'testslow'}" );
                    System.out.println(
                            "ReplSize:-1 cl write data must be error when node slow" );
                    Assert.fail(
                            "ReplSize:-1 cl write data must be error when node slow" );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -105
                            && e.getErrorCode() != -252 ) {
                        throw e;
                    }
                }

            } finally {
                shutoff = true;
            }
        }
    }
}
