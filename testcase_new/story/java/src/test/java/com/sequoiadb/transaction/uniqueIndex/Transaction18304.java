package com.sequoiadb.transaction.uniqueIndex;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName seqDB-18304 : 存在自增字段及唯一索引，插入索引键重复记录
 * @Author zhaoyu
 * @Date 2019年4月24日
 */
@Test(groups = { "rc", "ru", "rcuserbs" })
public class Transaction18304 extends SdbTestBase {

    private String clName = "transCL_18304";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", true, false );
    }

    @Test
    public void test() {
        sdb.beginTransaction();

        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{a:1,b:1,c:1}" );
        cl.insert( insertR1 );
        try {
            cl.insert( insertR1 );
            Assert.fail( "can not insert duplicate key" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38 );
        } finally {
            sdb.commit();
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

}
