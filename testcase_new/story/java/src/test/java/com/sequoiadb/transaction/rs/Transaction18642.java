package com.sequoiadb.transaction.rs;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-18642:批量删除记录与查询并发，事务提交
 * @date 2019-7-9
 * @author yinzhen
 *
 */
@Test(groups = { "rs" })
public class Transaction18642 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private String hashCLName = "cl18642_hash";
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'idx18642'}";
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "ONE GROUP MODE" );
        }

        // 创建分区表并插入记录R1s
        TransUtils.createHashCL( sdb, csName, hashCLName );
        cl = sdb.getCollectionSpace( csName ).getCollection( hashCLName );
        TransUtils.insertRandomDatas( cl, 0, 10000 );
        cl.createIndex( "idx18642", "{a:1}", false, false );
    }

    @AfterClass
    public void tearDown() {
        if ( db1 != null ) {
            db1.commit();
            db1.close();
        }
        if ( db2 != null ) {
            db2.commit();
            db2.close();
        }
        if ( sdb != null ) {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( hashCLName );
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启两个并发事务
        db1.beginTransaction();
        db2.beginTransaction();
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( hashCLName );
        DBCollection cl2 = db2.getCollectionSpace( csName )
                .getCollection( hashCLName );

        // 事务1批量删除记录后为R2s
        cl1.delete( null, hintIxScan );

        // 事务2表扫描/索引扫描记录
        CL2Query th2_1 = new CL2Query( hintTbScan,
                new ArrayList< BSONObject >() );
        th2_1.start();
        Assert.assertTrue( th2_1.matchBlockingMethod( DBCursor.class.getName(),
                "hasNext" ) );

        CL2Query th2_2 = new CL2Query( hintIxScan,
                new ArrayList< BSONObject >() );
        th2_2.start();
        Assert.assertTrue( th2_2.matchBlockingMethod( DBCursor.class.getName(),
                "hasNext" ) );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", hintTbScan,
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl, "{a:1}", hintIxScan,
                new ArrayList< BSONObject >() );

        // 事务1提交
        db1.commit();

        Assert.assertTrue( th2_1.isSuccess(), th2_1.getErrorMsg() );
        Assert.assertTrue( th2_2.isSuccess(), th2_2.getErrorMsg() );

        // 事务2表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl2, "{a:1}", hintTbScan,
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:1}", hintIxScan,
                new ArrayList< BSONObject >() );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", hintTbScan,
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl, "{a:1}", hintIxScan,
                new ArrayList< BSONObject >() );

        // 事务2提交
        db2.commit();
    }

    private class CL2Query extends SdbThreadBase {
        private String hint;
        private List< BSONObject > expList;

        private CL2Query( String hint, List< BSONObject > expList ) {
            this.hint = hint;
            this.expList = expList;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db2 = null;
            try {
                db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db2.beginTransaction();
                DBCollection cl2 = db2.getCollectionSpace( csName )
                        .getCollection( hashCLName );
                TransUtils.queryAndCheck( cl2, "{a:1}", hint, expList );
                db2.commit();
            } finally {
                db2.rollback();
                db2.close();
            }
        }
    }
}
