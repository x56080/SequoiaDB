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
 * @testcase seqDB-17149:同一事务下，插入记录后读记录
 * @date 2019-1-22
 * @author yinzhen
 *
 */
@Test(groups = "rcwaitlock")
public class Transaction17149 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17149";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > actList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "textIndex17149", "{a:1}", false, false );
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
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        cl.insert( record );
        expList.add( record );

        // 读记录走表扫描
        DBCursor recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 读记录走索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17149'}" );
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
                "{'':'textIndex17149'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 删除记录
        cl.delete( "", "{'':'textIndex17190'}" );

        // 读记录走表扫描
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        expList.clear();
        Assert.assertEquals( actList, expList );

        // 读记录走索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17149'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );
        recordsCursor.close();
    }
}
