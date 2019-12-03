package com.sequoiadb.transaction.rc;

/**
 * @Description seqDB-17088:事务中批量插入与读并发 
 * @author xiaoni Zhao
 * @date 2019-1-21
 */
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rc")
public class Transaction17088 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private String hashCLName = "cl17088_hash";
    private String mainCLName = "cl17088_main";
    private String subCLName1 = "subcl17088_1";
    private String subCLName2 = "subcl17088_2";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "ONE GROUP MODE" );
        }

        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        TransUtils.createCLs( sdb, csName, hashCLName, mainCLName, subCLName1,
                subCLName2, 25000 );
    }

    @DataProvider(name = "getCL")
    private Object[][] getCLName() {
        return new Object[][] { { hashCLName }, { mainCLName } };
    }

    @Test(dataProvider = "getCL")
    public void test( String clName ) {
        cl = sdb.getCollectionSpace( csName ).getCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );

        // 开启两个并发事务
        db1.beginTransaction();
        db2.beginTransaction();

        // 事务1执行批量插入记录
        expList = TransUtils.insertDatas( cl1, 0, 50000, 1 );

        // 事务2表扫描记录
        TransUtils.queryAndCheck( cl2, "{'':null}",
                new ArrayList< BSONObject >() );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, "{'':'a'}",
                new ArrayList< BSONObject >() );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'a'}", expList );

        db1.commit();

        // 事务2表扫描记录
        TransUtils.queryAndCheck( cl2, "{_id:1}", "{'':null}", expList );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, "{_id:1}", "{'':'a'}", expList );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'a'}", expList );

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
        if ( cs.isCollectionExist( hashCLName ) ) {
            cs.dropCollection( hashCLName );
        }
        if ( cs.isCollectionExist( mainCLName ) ) {
            cs.dropCollection( mainCLName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
