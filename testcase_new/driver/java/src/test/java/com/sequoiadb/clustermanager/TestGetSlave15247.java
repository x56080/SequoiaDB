package com.sequoiadb.clustermanager;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;

/**
 * @TestLink: seqDB-15247
 * @describe:use interfaces as follow: 1. getSlave(int... positions)
 * @author wuyan
 * @Date 2018.5.3
 * @version 1.00
 */
public class TestGetSlave15247 extends SdbTestBase {
    private Sequoiadb sdb;
    private SimpleDateFormat df = new SimpleDateFormat(
            "YYYY-MM-dd HH:mm:ss.SSS" );
    private String coordAddr;
    private CommLib commlib = new CommLib();

    @BeforeClass
    public void setup() {
        this.coordAddr = SdbTestBase.coordUrl;
        try {
            System.out.println( "the TestCase: " + this.getClass().getName()
                    + " begin at:" + df.format( new Date().getTime() ) );
            sdb = new Sequoiadb( coordAddr, "", "" );

            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException(
                        "run mode is standalone,test case skip" );
            }

        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void teardown() {
        System.out.println( "the TestCase: " + this.getClass().getName()
                + " end at:" + df.format( new Date().getTime() ) );

        sdb.close();
    }

    @Test
    public void test() {
        ArrayList< String > dataGroupNames = commlib.getDataGroupNames( sdb );
        ReplicaGroup rGroup = sdb.getReplicaGroup( dataGroupNames.get( 0 ) );
        Collection< Integer > positionslist = new ArrayList< Integer >();
        positionslist.add( 1 );
        positionslist.add( 2 );
        positionslist.add( 3 );
        Node node = rGroup.getSlave( positionslist );
        Node masterNode = rGroup.getMaster();
        Assert.assertNotEquals( node, masterNode, "masternode is:"
                + masterNode.getNodeId() + "  slave node:" + node.getNodeId() );
    }
}
