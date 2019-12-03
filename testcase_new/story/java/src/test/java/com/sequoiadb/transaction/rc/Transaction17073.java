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
 * @testcase seqDB-17073:同一事务下，删除记录后读记录
 * @date 2019-1-18
 * @author yinzhen
 *
 */
@Test(groups = "rc")
public class Transaction17073 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17073";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'textIndex17073'}";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "textIndex17073", "{a:1}", false, false );
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

        // 删除记录R1
        cl.delete( "", hintIxScan );

        // 读记录走表扫描
        expList.clear();
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
    }
}
