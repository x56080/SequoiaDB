package com.sequoiadb.sessionaccess;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-14146
 * @describe: 设置会话访问属性指定实例为instanceid和[M/S/A]
 * @author wangkexin
 * @Date 2019.02.16
 * @version 1.00
 */
public class SessionAccess14146 extends SdbTestBase {
    private String clname = "cl14146";
    private Sequoiadb db;
    private DBCollection dbcl;
    private BasicBSONList nodes;
    private String rgName = "sessionAccessRG14146";

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

    @SuppressWarnings({ "rawtypes", "unchecked" })
    @Test
    public void test14146() {
        List< Integer > instanceidList = SessionAccessUtil
                .getInstanceidList( nodes );

        // 指定多个instanceid实例和"M"
        List currPreferedInstanceM = new ArrayList( instanceidList );
        currPreferedInstanceM.add( "M" );
        BasicBSONObject options = new BasicBSONObject( "PreferedInstance",
                currPreferedInstanceM ).append( "PreferedInstanceMode",
                        "ordered" );

        db.setSessionAttr( options );
        String actualNodeName = SessionAccessUtil.getActualDataNodeName( dbcl );
        Assert.assertTrue(
                SessionAccessUtil.isMaster( db, rgName, actualNodeName ),
                "testa: current node name is : " + actualNodeName
                        + "\n db.getSessionAttr():" + db.getSessionAttr() );

        // 指定多个instanceid实例和"S"
        List currPreferedInstanceS = new ArrayList( instanceidList );
        currPreferedInstanceS.add( "S" );
        options = new BasicBSONObject( "PreferedInstance",
                currPreferedInstanceS ).append( "PreferedInstanceMode",
                        "ordered" );
        db.setSessionAttr( options );
        actualNodeName = SessionAccessUtil.getActualDataNodeName( dbcl );
        Assert.assertFalse(
                SessionAccessUtil.isMaster( db, rgName, actualNodeName ),
                "testc: current node name is : " + actualNodeName
                        + "\n db.getSessionAttr():" + db.getSessionAttr() );

        // 指定多个instanceid实例和"m","s","a","A"
        String[] ids = new String[] { "m", "s", "a", "A" };
        for ( String id : ids ) {
            List currPreferedInstance = new ArrayList( instanceidList );
            currPreferedInstance.add( id );
            options = new BasicBSONObject( "PreferedInstance",
                    currPreferedInstance ).append( "PreferedInstanceMode",
                            "ordered" );
            db.setSessionAttr( options );
            actualNodeName = SessionAccessUtil.getActualDataNodeName( dbcl );
            String expNodeName = SessionAccessUtil.getNodeNameByInstanceId(
                    nodes, instanceidList.get( 0 ).toString() );
            Assert.assertEquals( actualNodeName, expNodeName );
        }
    }

    @AfterClass
    public void teardown() throws InterruptedException {
        db.getCollectionSpace( SdbTestBase.csName ).dropCollection( clname );
        db.removeReplicaGroup( rgName );
        db.close();
    }
}
