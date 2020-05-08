package com.sequoiadb.transaction.ru;

/**
 * @Description seqDB-17207:事务中批量更新与读并发 
 * @author xiaoni Zhao
 * @date 2019-1-17
 */
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "ru")
public class Transaction17207 extends SdbTestBase {
    private String clName = "cl_17207";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private String hintIxScan = "{'':'a'}";
    private String hintTbScan = "{'':null}";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        TransUtils.insertDatas( cl, 0, 50000, 1 );
    }

    @Test
    public void test() {
        // 开启两个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 事务1执行批量更新
        cl1.update( "{a:1}", "{$set:{a:2}}", hintIxScan );
        expList = TransUtils.getUpdateDatas( 0, 50000, 2 );

        // 事务2表扫描记录
        TransUtils.queryAndCheck( cl2, "{_id:1}", hintTbScan, expList );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, "{_id:1}", hintIxScan, expList );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", hintTbScan, expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", hintIxScan, expList );

        db1.commit();

        // 事务2表扫描记录
        TransUtils.queryAndCheck( cl2, "{_id:1}", hintTbScan, expList );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, "{_id:1}", hintIxScan, expList );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", hintTbScan, expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", hintIxScan, expList );

        db2.commit();
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }
}
