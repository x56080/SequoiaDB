package com.sequoiadb.sessionaccess;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.fail;

/**
 * @TestLink: seqDB-14145
 * @describe: 设置会话访问属性指定多个instanceid，其中节点选择模式为随机选取
 * @author wangkexin
 * @Date 2019.02.16
 * @version 1.00
 */
public class SessionAccess14145 extends SdbTestBase {
    private String clname = "cl14145";
    private Sequoiadb db;
    private DBCollection dbcl;
    private BasicBSONList nodes;
    private String rgName = "sessionAccessRG14145";

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }
        nodes = SessionAccessUtil.createRG( db, rgName );
        BSONObject options = new BasicBSONObject( "Group", rgName );
        options.put( "ReplSize", 0 );
        dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clname, options );
        SessionAccessUtil.insertRecords( dbcl );
    }

    @Test
    public void test14145() {
        List< Integer > instanceidList = SessionAccessUtil
                .getInstanceidList( nodes );
        int[] id = new int[] { instanceidList.get( 0 ),
                instanceidList.get( 1 ) };

        BSONObject options = new BasicBSONObject( "PreferedInstance", id )
                .append( "PreferedInstanceMode", "random" );
        db.setSessionAttr( options );
        String actualNodeName = SessionAccessUtil.getActualDataNodeName( dbcl );
        String expNodeName1 = SessionAccessUtil.getNodeNameByInstanceId( nodes,
                instanceidList.get( 0 ).toString() );
        String expNodeName2 = SessionAccessUtil.getNodeNameByInstanceId( nodes,
                instanceidList.get( 1 ).toString() );
        if ( !actualNodeName.equals( expNodeName1 )
                && !actualNodeName.equals( expNodeName2 ) ) {
            fail( "actual node name :" + actualNodeName + " expect node name : "
                    + expNodeName1 + " or " + expNodeName2 );
        }
        BSONObject actualSessionAttr = db.getSessionAttr();
        BasicBSONList actualIdList = ( BasicBSONList ) actualSessionAttr
                .get( "PreferedInstance" );
        BasicBSONList expectIdList = new BasicBSONList();
        for ( int i : id ) {
            expectIdList.add( i );
        }
        assertEquals( actualIdList, expectIdList );
        assertEquals( actualSessionAttr.get( "PreferedInstanceMode" ),
                "random" );
    }

    @AfterClass
    public void teardown() throws InterruptedException {
        db.getCollectionSpace( SdbTestBase.csName ).dropCollection( clname );
        db.removeReplicaGroup( rgName );
        db.close();
    }
}
