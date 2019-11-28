package com.sequoiadb.transaction.brokennetwork;

import java.util.List;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;
import com.sequoiadb.transaction.common.TransferTh;

/**
 * @Description seqDB-18825:hash分区表/主子表，转账的过程中coord节点断网
 * @author yinzhen
 * @date 2019-7-17
 *
 */
@Test(groups = "rcauto")
public class Transaction18825 extends SdbTestBase {
    private Sequoiadb sdb;
    private String hashCLName = "cl18825_hash";
    private String mainCLName = "cl18825_main";
    private String subCLName1 = "subcl18825_1";
    private String subCLName2 = "subcl18825_2";
    private GroupMgr groupMgr;
    private List< String > groupNames;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException( "ONE GROUP MODE" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120 ) ) {
            throw new SkipException( "GROUP ERROR" );
        }

        // 创建hash分区表/主子表(主表下挂载多个子表，子表覆盖分区表)，replSize设置为1，且已切分到所有组上，切分键为账户字段
        // 并插入数据 10000 个账户，每个账户 10000 元
        TransUtil.createCLsAndInsertData( sdb, csName, hashCLName, mainCLName,
                subCLName1, subCLName2 );
    }

    @AfterClass
    public void tearDown() throws InterruptedException {
        TransUtil.cleanEnv( sdb, csName, hashCLName, mainCLName );
    }

    @DataProvider(name = "getCL")
    private Object[][] getCLName() {
        return new Object[][] { { hashCLName }, { mainCLName } };
    }

    @Test(dataProvider = "getCL")
    public void test( String clName )
            throws ReliabilityException, InterruptedException {
        // 转账程序连接的coord节点断网
        TaskMgr taskMgr = new TaskMgr();
        String coordUrl = TransUtil.getCoordUrl( sdb );
        NodeWrapper coordNode = TransUtil
                .getCoordNode( new Sequoiadb( coordUrl, "", "" ) );
        FaultMakeTask task = BrokenNetwork
                .getFaultMakeTask( coordNode.hostName(), 60, 10 );
        taskMgr.addTask( task );
        TransUtil.setTimeTask( taskMgr, task );

        for ( int i = 0; i < 200; i++ ) {
            taskMgr.addTask( new TransferTh( csName, clName, coordUrl, true ) );
        }
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 300 ),
                "GROUP ERROR" );

        // 待集群正常后，查询所有账户的金额总和
        int count = 0;
        int checkTimes = 600;
        while ( count++ < checkTimes ) {
            DBCursor cursor = sdb.exec( "select sum(balance) as balance from "
                    + csName + "." + clName );
            double balance = ( double ) cursor.getNext().get( "balance" );
            cursor.close();
            if ( 100000000 != ( int ) balance ) {
                if ( count == checkTimes ) {
                    System.out.println(
                            "Transaction18518 amount of general ledger: "
                                    + balance );
                    Assert.fail( "check amount of general ledger timeout..." );
                }
                Thread.sleep( 1000 );
                continue;
            }
            break;
        }
    }
}
