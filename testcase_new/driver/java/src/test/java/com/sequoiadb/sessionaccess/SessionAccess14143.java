package com.sequoiadb.sessionaccess;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import static org.testng.Assert.*;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * @TestLink: seqDB-14143
 * @describe: 设置会话访问属性，单值指定访问实例为M/S/A
 * @author wangkexin
 * @Date 2019.02.16
 * @version 1.00
 */
public class SessionAccess14143 extends SdbTestBase {
    private String clname = "cl14143";
    private Sequoiadb db;
    private DBCollection dbcl;
    private BasicBSONList nodes;
    private String rgName = "sessionAccessRG14143";

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
    public void test14143() {
        String[] expectPreferedInstance = new String[] { "M", "S" };
        for ( String s : expectPreferedInstance ) {
            BasicBSONObject options = new BasicBSONObject( "PreferedInstance",
                    s );
            db.setSessionAttr( options );
            String hostName = SessionAccessUtil.getActualDataNodeName( dbcl );
            if ( s.equals( "M" ) ) {
                assertTrue( SessionAccessUtil.isMaster( db, rgName, hostName ),
                        "the actual data node name is: " + hostName
                                + ",the current option is "
                                + options.toString() );
            } else if ( s.equals( "S" ) ) {
                assertFalse( SessionAccessUtil.isMaster( db, rgName, hostName ),
                        "the actual data node name is: " + hostName
                                + ",the current option is "
                                + options.toString() );
            }

            String expPreferedInstance = options.getString( "PreferedInstance" )
                    .toString();

            BSONObject actSessionAttr = db.getSessionAttr();
            String actPreferedInstance = actSessionAttr
                    .get( "PreferedInstance" ).toString();
            assertEquals( actPreferedInstance, expPreferedInstance );
        }

        List< String > nodeNames = new ArrayList< String >();
        for ( int i = 0; i < nodes.size(); i++ ) {
            BasicBSONObject node = ( BasicBSONObject ) nodes.get( i );
            nodeNames.add( node.getString( "nodeName" ) );
        }
        // 设置PreferedInstance为'A'
        BasicBSONObject options = new BasicBSONObject( "PreferedInstance",
                "A" );
        Set< String > actNodeNames = new HashSet< String >();
        for ( int i = 0; i < 20; i++ ) {
            db.setSessionAttr( options );
            String actNodeName = SessionAccessUtil
                    .getActualDataNodeName( dbcl );
            Assert.assertTrue( nodeNames.contains( actNodeName ),
                    "The actual Node name is not expected: " + actNodeName );
            actNodeNames.add( actNodeName );
        }

        // Set中存放的是不重复的元素，如果actNodeNames大小为1时，代表实际操作的节点只有一个，没有随机取值
        Assert.assertNotEquals( actNodeNames.size(), 1,
                "When PreferedInstance is 'A', the actual node is unchanged, the node name is:"
                        + actNodeNames.iterator().next() );

        String expPreferedInstance = options.getString( "PreferedInstance" )
                .toString();

        BSONObject actSessionAttr = db.getSessionAttr();
        String actPreferedInstance = actSessionAttr.get( "PreferedInstance" )
                .toString();
        assertEquals( actPreferedInstance, expPreferedInstance );
    }

    @AfterClass
    public void teardown() throws InterruptedException {
        db.getCollectionSpace( SdbTestBase.csName ).dropCollection( clname );
        db.removeReplicaGroup( rgName );
        db.close();
    }
}
