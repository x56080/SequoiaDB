package com.sequoiadb.transaction.rc;

/**
 * @Description seqDB-17090: 事务中批量删除与读并发 
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
public class Transaction17090 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private String hashCLName = "cl17090_hash";
    private String mainCLName = "cl17090_main";
    private String subCLName1 = "subcl17090_1";
    private String subCLName2 = "subcl17090_2";
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'a'}";

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
        expList = TransUtils.insertDatas( cl, 0, 50000, 1 );

        // 开启两个并发事务
        db1.beginTransaction();
        db2.beginTransaction();

        // 事务1批量删除记录
        cl1.delete( "{a:1}", hintIxScan );

        // 事务2表扫描记录
        TransUtils.queryAndCheck( cl2, "{_id:1}", hintTbScan, expList );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, "{_id:1}", hintIxScan, expList );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, hintTbScan,
                new ArrayList< BSONObject >() );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, hintIxScan,
                new ArrayList< BSONObject >() );

        db1.commit();

        // 事务2表扫描记录
        TransUtils.queryAndCheck( cl2, hintTbScan,
                new ArrayList< BSONObject >() );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, hintIxScan,
                new ArrayList< BSONObject >() );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, hintTbScan,
                new ArrayList< BSONObject >() );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, hintIxScan,
                new ArrayList< BSONObject >() );

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
