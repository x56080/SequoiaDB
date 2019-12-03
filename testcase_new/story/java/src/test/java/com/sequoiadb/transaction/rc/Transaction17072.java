package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17072:同一事务下，更新记录后读记录
 * @date 2019-1-17
 * @author yinzhen
 *
 */
@Test(groups = "rc")
public class Transaction17072 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17072";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'textIndex17072'}";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "textIndex17072", "{a:1}", false, false );
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        cl.insert( record );
    }

    @AfterClass
    public void tearDown() {
        sdb.commit();
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启事务
        sdb.beginTransaction();

        // 更新索引字段值
        cl.update( "{a:1}", "{$set:{a:2}}", hintIxScan );
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" ) );

        // 读记录走表扫描,查询条件使用更新后值
        TransUtils.queryAndCheck( cl, "{a:2}", null, null, hintTbScan,
                expList );

        // 读记录走索引扫描,查询条件使用更新后值
        TransUtils.queryAndCheck( cl, "{a:2}", null, null, hintIxScan,
                expList );

        // 读记录走表扫描,查询条件使用更新前值
        TransUtils.queryAndCheck( cl, "{a:1}", null, null, hintTbScan,
                new ArrayList< BSONObject >() );

        // 读记录走索引扫描,查询条件使用更新前值
        TransUtils.queryAndCheck( cl, "{a:1}", null, null, hintIxScan,
                new ArrayList< BSONObject >() );

        // 记录删除索引字段
        cl.update( "{a:2}", "{$unset:{a:''}}", hintIxScan );
        expList.clear();
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, b:1}" ) );

        // 读记录走表扫描
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 读记录走索引扫描
        TransUtils.queryAndCheck( cl, "{b:{$exists:1}}", null, null, hintIxScan,
                expList );

        // 记录新增索引字段
        cl.update( "{b:1}", "{$set:{a:3}}", hintIxScan );
        expList.clear();
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, a:3, b:1}" ) );

        // 读记录走表扫描
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 读记录走索引扫描
        TransUtils.queryAndCheck( cl, "{a:{$exists:1}}", null, null, hintIxScan,
                expList );

        // 记录新增非索引字段
        cl.update( "{a:3}", "{$set:{c:1}}", hintIxScan );
        expList.clear();
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, a:3, b:1, c:1}" ) );

        // 读记录走表扫描
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 读记录走索引扫描
        TransUtils.queryAndCheck( cl, "{a:{$exists:1}}", null, null, hintIxScan,
                expList );

        // 事务提交
        sdb.commit();

        // 读记录走表扫描
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 读记录走索引扫描
        TransUtils.queryAndCheck( cl, "{a:{$exists:1}}", null, null, hintIxScan,
                expList );

        // 删除记录
        cl.delete( "", hintIxScan );
    }
}
