package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-22232:coord/data节点发起写事务，data节点发起读事务
 * @date 2020-06-02
 * @author luweikang
 *
 */
@Test(groups = { "rrauto" })
public class Transaction22232 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb TW = null;
    private String clName = "cl22232";
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }

        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName );
        cl.createIndex( "a", "{a:1}", true, false );
    }

    @Test
    public void test() {
        TW = CommLib.getRandomSequoiadb();

        DBCollection clTW = TW.getCollectionSpace( csName )
                .getCollection( clName );

        BSONObject record1 = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        BSONObject record2 = ( BSONObject ) JSON.parse( "{_id:2, a:2, b:2}" );
        expList.add( record1 );

        try {
            TW.beginTransaction();
            clTW.insert( record1 );
            TransUtils.commitTransaction( TW );

            TransUtils.queryAndCheck( clTW, "{'':null}", expList );
            TransUtils.queryAndCheck( clTW, "{'':'a'}", expList );

            clTW.insert( record2 );
            expList.add( record2 );

            TransUtils.queryAndCheck( clTW, "{a: 1}", "{'':null}", expList );
            TransUtils.queryAndCheck( clTW, "{a: 1}", "{'':'a'}", expList );
        } finally {
            TransUtils.commitTransaction( TW );
        }

    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
        sdb.close();
        TW.close();
    }
}
