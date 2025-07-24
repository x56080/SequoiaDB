package com.sequoiadb.transaction.rr;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20511:不同session混合不同隔离级别下读写并发
 * @date 2020-1-22
 * @author Lena
 * 
 */
@Test(groups = "rr")
public class Transaction20511 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String clName = "cl20511";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > expList2 = new ArrayList< BSONObject >();
    private BSONObject record1 = new BasicBSONObject();
    private BSONObject record2 = new BasicBSONObject();

    @BeforeClass
    public void setUp() {

        sdb = CommLib.getRandomSequoiadb();
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );

        record1 = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        record2 = ( BSONObject ) JSON.parse( "{_id:2, a:2, b:2}" );

        cl.insert( record1 );
        cl.insert( record2 );
        expList.add( record1 );
        expList.add( record2 );
    }

    @Test
    public void test() {

        Sequoiadb sdb1 = CommLib.getRandomSequoiadb();
        DBCollection cl1 = sdb1.getCollectionSpace( csName )
                .getCollection( clName );
        Sequoiadb sdb2 = CommLib.getRandomSequoiadb();
        DBCollection cl2 = sdb2.getCollectionSpace( csName )
                .getCollection( clName );
        Sequoiadb sdb3 = CommLib.getRandomSequoiadb();
        DBCollection cl3 = sdb3.getCollectionSpace( csName )
                .getCollection( clName );
        Sequoiadb sdbw = CommLib.getRandomSequoiadb();
        DBCollection clw = sdbw.getCollectionSpace( csName )
                .getCollection( clName );

        try {
            // 配置session为RU隔离级别，开启读事务TR1

            sdb1.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransIsolation:0}" ) );
            BSONObject attr1 = sdb1.getSessionAttr();
            Assert.assertEquals( 0, attr1.get( "TransIsolation" ) );
            TransUtils.beginTransaction( sdb1 );

            // 配置session为RC隔离级别，开启读事务TR2

            sdb2.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransIsolation:1}" ) );
            BSONObject attr2 = sdb2.getSessionAttr();
            Assert.assertEquals( 1, attr2.get( "TransIsolation" ) );
            TransUtils.beginTransaction( sdb2 );

            // 配置session为RR隔离级别，开启读事务TR3

            sdb3.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransIsolation:3}" ) );
            BSONObject attr3 = sdb3.getSessionAttr();
            Assert.assertEquals( 3, attr3.get( "TransIsolation" ) );
            TransUtils.beginTransaction( sdb3 );

            // 开启写事务,插入R3，更新R1，删除R2

            TransUtils.beginTransaction( sdbw );

            BSONObject insertR3 = ( BSONObject ) JSON
                    .parse( "{_id:3,a:3,b:3}" );
            clw.update( "{a:1}", "{$set:{b:4}}", "{'':null}" );
            clw.delete( "{a:2}", "{'':null}" );
            clw.insert( insertR3 );

            expList2.add( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:4}" ) );
            expList2.add( insertR3 );

            // 所有读事务读记录
            TransUtils.queryAndCheck( cl1, null, null, "{'':null}", expList2 );
            TransUtils.queryAndCheck( cl2, null, null, "{'':null}", expList );
            TransUtils.queryAndCheck( cl3, null, null, "{'':null}", expList );

            // 提交写事务TW1
            TransUtils.commitTransaction( sdbw );

            // 所有读事务读记录
            TransUtils.queryAndCheck( cl1, null, null, "{'':null}", expList2 );
            TransUtils.queryAndCheck( cl2, null, null, "{'':null}", expList2 );
            TransUtils.queryAndCheck( cl3, null, null, "{'':null}", expList );
        } finally {

            TransUtils.commitTransaction( sdb1 );
            TransUtils.commitTransaction( sdb2 );
            TransUtils.commitTransaction( sdb3 );
            TransUtils.commitTransaction( sdbw );

            if ( !sdb1.isClosed() ) {
                sdb1.close();
            }

            if ( !sdb2.isClosed() ) {
                sdb2.close();
            }

            if ( !sdb3.isClosed() ) {
                sdb3.close();
            }
            if ( !sdbw.isClosed() ) {
                sdbw.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {

        CollectionSpace cs = sdb.getCollectionSpace( csName );

        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }

    }

}
