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
 * @testcase seqDB-21985:写操作后，显示开启事务查询
 * @date 2020-3-25
 * @author zhaoyu
 *
 */
@Test(groups = { "rrauto" })
public class Transaction21985 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db = null;
    private String clName = "cl21985";
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        db = CommLib.getRandomSequoiadb();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less than two groups" );
        }

        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName );
        cl.createIndex( "a", "{a:1}", true, false );
    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
        sdb.close();
        db.close();
    }

    @Test
    public void test() throws InterruptedException {
        try {
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:1, a:1, b:1}" );
            expList.add( record );
            cl.insert( record );

            Thread.sleep( 100 );
            // 开启另1个连接,开启事务执行查询并提交
            DBCollection cl1 = db.getCollectionSpace( csName )
                    .getCollection( clName );
            TransUtils.beginTransaction( db );
            TransUtils.queryAndCheck( cl1, "{'':null}", expList );
            TransUtils.queryAndCheck( cl1, "{'':'a'}", expList );

        } finally {
            TransUtils.commitTransaction( db );
        }

    }

}
