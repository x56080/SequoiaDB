package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17142:固定集合中增删改记录，与读并发
 * @date 2019-1-24
 * @author yinzhen
 */
@Test(groups = "rc")
public class Transaction17142 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cappedCS17142";
    private String clName = "cappedCL17142";
    private DBCollection cappedCL = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private BSONObject object = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cappedCL = sdb
                .createCollectionSpace( csName,
                        ( BSONObject ) JSON.parse( "{Capped:true}" ) )
                .createCollection( clName, ( BSONObject ) JSON
                        .parse( "{Capped:true, Size:1024}" ) );
        object = new BasicBSONObject();
        long oid = 0L;
        object.put( "_id", oid );
        object.put( "a", 1 );
        object.put( "b", 1 );
        cappedCL.insert( object );
        expList.add( object );
        object = new BasicBSONObject();
        oid = 64L;
        object.put( "_id", oid );
        object.put( "a", 2 );
        object.put( "b", 2 );
        cappedCL.insert( object );
        expList.add( object );
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
        if ( !sdb.isClosed() ) {
            sdb.dropCollectionSpace( csName );
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启并发事务
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        db1.beginTransaction();
        db2.beginTransaction();

        // 事务1插入记录，并读记录走表扫描
        long oid = 128L;
        BSONObject object = new BasicBSONObject();
        object.put( "_id", oid );
        object.put( "a", 3 );
        object.put( "b", 3 );
        cl1.insert( object );
        expList.add( object );
        TransUtils.queryAndCheck( cl1, "{a:1}", "{'':null}", expList );

        // 事务2读记录走表扫描
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expList );

        // 事务1执行pop操作
        cl1.pop( ( BSONObject ) JSON.parse( "{LogicalID:0, Direction:1}" ) );
        cl1.pop( ( BSONObject ) JSON
                .parse( "{LogicalID:" + oid + ", Direction:-1}" ) );
        expList.clear();
        expList.add( this.object );

        // 事务1读记录走表扫描
        TransUtils.queryAndCheck( cl1, "{a:1}", "{'':null}", expList );

        // 事务2读记录走表扫描
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expList );

        // 回滚事务1，并读记录走表扫描
        db1.rollback();
        TransUtils.queryAndCheck( cl1, "{a:1}", "{'':null}", expList );

        // 事务2读记录走表扫描
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expList );

        // 事务2回滚
        db2.rollback();
    }
}
