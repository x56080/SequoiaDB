package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-22322:老版本记录位于链头
 * @date 2020-06-16
 * @author zhaoyu
 */
@Test(groups = "rr")
public class Transaction22322 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private String clName = "cl_22322";
    private String idxName = "index_22322";
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private BSONObject record = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = CommLib.getRandomSequoiadb();
        db1 = CommLib.getRandomSequoiadb();
        db2 = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( idxName, "{ a: 1 }", false, false );
        record = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        cl.insert( record );
    }

    @Test
    public void test() {
        // 连接1上事务中更新记录
        TransUtils.beginTransaction( db1 );
        cl1.update( null, "{$inc:{a:1}}", null );
        TransUtils.commitTransaction( db1 );

        // 事务2上查询
        BSONObject record1 = ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" );
        expList.add( record1 );
        TransUtils.beginTransaction( db2 );
        TransUtils.queryAndCheck( cl2, "", "", "{'':'" + idxName + "'}",
                expList );
        TransUtils.queryAndCheck( cl2, "", "", "{'':null}", expList );
        TransUtils.queryAndCheck( cl2, "{a:1}", "", "{'':'" + idxName + "'}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:2}", "", "{'':'" + idxName + "'}",
                expList );
        TransUtils.queryAndCheck( cl2, "{a:3}", "", "{'':'" + idxName + "'}",
                new ArrayList< BSONObject >() );

        // 非事务remove
        cl1.delete( "" );

        // 事务2查询
        TransUtils.queryAndCheck( cl2, "", "", "{'':'" + idxName + "'}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "", "", "{'':null}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:1}", "", "{'':'" + idxName + "'}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:2}", "", "{'':'" + idxName + "'}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:3}", "", "{'':'" + idxName + "'}",
                new ArrayList< BSONObject >() );

        // 连接1上事务中插入记录
        TransUtils.beginTransaction( db1 );
        cl1.insert( record );
        TransUtils.commitTransaction( db1 );

        // TR1读，检查结果
        TransUtils.queryAndCheck( cl2, "", "", "{'':'" + idxName + "'}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "", "", "{'':null}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:1}", "", "{'':'" + idxName + "'}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:2}", "", "{'':'" + idxName + "'}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:3}", "", "{'':'" + idxName + "'}",
                new ArrayList< BSONObject >() );
    }

    @AfterClass
    public void tearDown() {
        // 提交事务
        db1.rollback();
        db1.close();

        db2.rollback();
        db1.close();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

}
