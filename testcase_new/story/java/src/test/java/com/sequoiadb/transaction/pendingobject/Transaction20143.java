package com.sequoiadb.transaction.pendingobject;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @FileName seqDB-20143:事务中插入并删除记录，其他事务插入同样的记录，回滚事务
 * @Author zhaoyu
 * @Date 2019年11月1日
 */
@Test
public class Transaction20143 extends SdbTestBase {

    private String clName = "transCL_20143";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private List< BSONObject > expDataList = new ArrayList< BSONObject >();
    private int insertNum = 100;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a20143", "{a:1}", true, false );
    }

    @Test
    public void test() {
        Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        Sequoiadb db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        try {
            // 事务中插入删除记录
            db1.beginTransaction();
            db2.beginTransaction();
            DBCollection tcl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            ArrayList< BSONObject > insertR1s = new ArrayList< BSONObject >();
            for ( int i = 0; i < insertNum; i++ ) {
                insertR1s.add( ( BSONObject ) JSON
                        .parse( "{_id:" + i + ",a:" + i + ",b:" + i + "}" ) );
            }
            tcl1.insert( insertR1s );
            int deleteNum = insertNum - 1;
            tcl1.delete( "{a:{$lt:" + deleteNum + "}}" );

            // 事务中插入记录，唯一索引值与插入记录的值相同
            DBCollection tcl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            for ( int i = 0; i < deleteNum; i++ ) {
                String record = "{_id:" + i + ",a:" + i + ",b:'insert20143'}";
                expDataList.add( ( BSONObject ) JSON.parse( record ) );
            }
            tcl2.insert( expDataList );
            db1.rollback();
            db2.rollback();

            // 校验结果
            List< String > groupNames = CommLib.getCLGroups( cl );
            String groupName = groupNames.get( 0 );
            Assert.assertTrue( TransUtils.isLsnConsistency( sdb, groupName ) );
            Assert.assertTrue(
                    TransUtils.getDatabaseSnapshot( sdb, groupName ) );
            TransUtils.queryAndCheck( cl, "{a:1}", "{a:''}",
                    new ArrayList< BSONObject >() );
        } finally {
            db1.commit();
            db2.commit();
            db1.close();
            db2.close();
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

}
