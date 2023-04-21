package com.sequoiadb.transaction.rr;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Descreption seqDB-31184:设置RR隔离级别，开启事务后通过索引查询数据
 *              seqDB-31186:设置RR隔离级别，开启事务后通过索进行count
 * @Author huanghaimei
 * @CreateDate 2023/4/7
 * @UpdateUser huanghaimei
 * @UpdateDate 2023/4/18
 * @UpdateRemark
 * @Version
 */

@Test(groups = { "rr" })
public class Transaction31184 extends SdbTestBase {
    private Sequoiadb db = null;
    private String clName = "cl_31184_31186";
    private DBCollection cl = null;
    private CollectionSpace cs;
    private int recsNum = 1000;
    private List< BSONObject > bulkInsertor;

    @BeforeClass
    public void setUp() throws InterruptedException {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = db.getCollectionSpace( csName ).createCollection( clName );

        // 创建索引
        BSONObject indexKeys = new BasicBSONObject();
        indexKeys.put( "a", 1 );
        BSONObject indexAttr = new BasicBSONObject();
        indexAttr.put( "NotArray", true );
        cl.createIndex( "a", indexKeys, indexAttr );

        // 插入数据
        bulkInsertor = new ArrayList<>();
        for ( int i = 0; i < recsNum; i++ ) {
            bulkInsertor
                    .add( new BasicBSONObject( "_id", i ).append( "a", i ) );
        }
        cl.bulkInsert( bulkInsertor );
    }

    @Test
    public void test() throws InterruptedException {
        TransUtils.beginTransaction( db );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", bulkInsertor );
        BSONObject opt = new BasicBSONObject();
        opt.put( "a", 1 );
        opt.put( "", 'a' );

        // 通过索引进行count(),查看TotalIndexRead、TotalIxScan字段值是否增加
        cl.getCount( opt );

        TransUtils.commitTransaction( db );

        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONS,
                new BasicBSONObject( "Name", SdbTestBase.csName + "." + clName )
                        .append( "RawData", true )
                        .append( "NodeSelect", "master" ),
                null, null );
        BSONObject info = cursor.getNext();
        BasicBSONList detail = ( BasicBSONList ) info.get( "Details" );
        long TotalIndexRead = ( long ) ( ( BasicBSONObject ) detail.get( 0 ) )
                .get( "TotalIndexRead" );
        Assert.assertEquals( TotalIndexRead, 1003 );
        long TotalIxScan = ( long ) ( ( BasicBSONObject ) detail.get( 0 ) )
                .get( "TotalIxScan" );
        Assert.assertEquals( TotalIxScan, 3 );
        cursor.close();

        queryByIndexAndCheckExplain( "{a:1}", "ixscan", true );
    }

    private void queryByIndexAndCheckExplain( String macher,
            String expectScanType, boolean expectIndexCover ) {
        // 检查扫描方式
        BSONObject sel = new BasicBSONObject();
        sel.put( "a", 1 );
        BSONObject hint = new BasicBSONObject();
        hint.put( "", "a" );
        DBCursor explainCursor = cl.explain(
                ( BSONObject ) JSON.parse( macher ), sel, null, hint, 0, 0, 0,
                null );
        if ( explainCursor.hasNext() ) {
            BSONObject obj = explainCursor.getNext();
            boolean IndexCover = ( boolean ) obj.get( "IndexCover" );
            Assert.assertEquals( IndexCover, expectIndexCover );
            String scanType = ( String ) obj.get( "ScanType" );
            Assert.assertEquals( scanType, expectScanType,
                    "scanType is " + scanType + " not" + expectScanType );
        } else {
            Assert.fail( "cl explain wrong" );
        }
        explainCursor.close();
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }
}