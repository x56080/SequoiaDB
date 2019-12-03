package com.sequoiadb.transaction.rcwaitlock;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17150:同一事务下，更新记录后读记录
 * @date 2019-1-22
 * @author yinzhen
 *
 */
@Test(groups = "rcwaitlock")
public class Transaction17150 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17150";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > actList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "textIndex17150", "{a:1}", false, false );
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
        cl.update( "{a:1}", "{$set:{a:2}}", "{'':'textIndex17150'}" );
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" ) );

        // 读记录走表扫描，使用更新前条件查询
        DBCursor recordsCursor = cl.query( "{a:1}", null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, new ArrayList< BSONObject >() );

        // 读记录走索引扫描，使用更新后条件查询
        recordsCursor = cl.query( "{a:1}", null, null,
                "{'':'textIndex17150'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, new ArrayList< BSONObject >() );

        // 读记录走表扫描，使用更新后条件查询
        recordsCursor = cl.query( "{a:2}", null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 读记录走索引扫描，使用更新后条件查询
        recordsCursor = cl.query( "{a:2}", null, null,
                "{'':'textIndex17150'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 记录删除索引字段
        cl.update( "{a:2}", "{$unset:{a:''}}", "{'':'textIndex17150'}" );
        expList.clear();
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, b:1}" ) );

        // 读记录走表扫描
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 读记录走索引扫描
        recordsCursor = cl.query( null, null, null, "{'':'textIndex17150'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 记录新增索引字段
        cl.update( "{b:1}", "{$set:{a:3}}", "{'':'textIndex17150'}" );
        expList.clear();
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, a:3, b:1}" ) );

        // 读记录走表扫描
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 读记录走索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17150'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 记录新增非索引字段
        cl.update( "{a:3}", "{$set:{c:1}}", "{'':'textIndex17150'}" );
        expList.clear();
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, a:3, b:1, c:1}" ) );

        // 读记录走表扫描
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 读记录走索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17150'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务提交
        sdb.commit();

        // 读记录走表扫描
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 读记录走索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17150'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 删除记录
        cl.delete( "", "{'':'textIndex17150'}" );

        // 读记录走表扫描
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        expList.clear();
        Assert.assertEquals( actList, expList );

        // 读记录走索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17150'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );
        recordsCursor.close();
    }
}
