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
 * @testcase seqDB-20512:同一个session进行不同隔离级别切换
 * @date 2020-1-27
 * @author Lena
 * 
 */
@Test(groups = "rr")
public class Transaction20512 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdbw1 = null;
    private Sequoiadb sdbw2 = null;
    private Sequoiadb sdbw3 = null;
    private String clName = "cl20512";
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection clw1 = null;
    private DBCollection clw2 = null;
    private DBCollection clw3 = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > expList2 = new ArrayList< BSONObject >();
    private List< BSONObject > expList3 = new ArrayList< BSONObject >();
    private List< BSONObject > expList4 = new ArrayList< BSONObject >();
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

        sdb1 = CommLib.getRandomSequoiadb();
        cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );

        sdbw1 = CommLib.getRandomSequoiadb();
        clw1 = sdbw1.getCollectionSpace( csName ).getCollection( clName );

        sdbw2 = CommLib.getRandomSequoiadb();
        clw2 = sdbw2.getCollectionSpace( csName ).getCollection( clName );

        sdbw3 = CommLib.getRandomSequoiadb();
        clw3 = sdbw3.getCollectionSpace( csName ).getCollection( clName );

        try {

            // 配置session为RR隔离级别，开启读事务TR1
            sdb1.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransIsolation:3}" ) );
            BSONObject attr1 = sdb1.getSessionAttr();
            Assert.assertEquals( 3, attr1.get( "TransIsolation" ) );
            TransUtils.beginTransaction( sdb1 );

            // 开启写事务,插入R3，更新R1，删除R2
            TransUtils.beginTransaction( sdbw1 );

            BSONObject insertR3 = ( BSONObject ) JSON
                    .parse( "{_id:3,a:3,b:3}" );
            clw1.update( "{a:1}", "{$set:{b:4}}", "{'':null}" );
            clw1.delete( "{a:2}", "{'':null}" );
            clw1.insert( insertR3 );

            expList2.add( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:4}" ) );
            expList2.add( insertR3 );

            TransUtils.commitTransaction( sdbw1 );

            // 读事务TR1读记录,并提交
            TransUtils.queryAndCheck( cl1, null, null, "{'':null}", expList );
            TransUtils.commitTransaction( sdb1 );

            // 配置同一session为RC隔离级别，开启读事务TR2
            sdb1.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransIsolation:1}" ) );
            BSONObject attr2 = sdb1.getSessionAttr();
            Assert.assertEquals( 1, attr2.get( "TransIsolation" ) );
            TransUtils.beginTransaction( sdb1 );

            // 开启写事务TW2,插入R3，更新R4，删除R3
            TransUtils.beginTransaction( sdbw2 );

            BSONObject insertR5 = ( BSONObject ) JSON
                    .parse( "{_id:5,a:5,b:5}" );
            clw2.update( "{a:1}", "{$set:{b:6}}", "{'':null}" );
            clw2.delete( "{a:3}", "{'':null}" );
            clw2.insert( insertR5 );

            expList3.add( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:6}" ) );
            expList3.add( insertR5 );

            // 读事务TR2读记录
            TransUtils.queryAndCheck( cl1, null, null, "{'':null}", expList2 );

            // 写事务TW2提交
            TransUtils.commitTransaction( sdbw2 );

            // 读事务TR2读记录
            TransUtils.queryAndCheck( cl1, null, null, "{'':null}", expList3 );
            TransUtils.commitTransaction( sdb1 );

            // 配置同一session为RU隔离级别，开启读事务TR3
            sdb1.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransIsolation:0}" ) );
            BSONObject attr3 = sdb1.getSessionAttr();
            Assert.assertEquals( 0, attr3.get( "TransIsolation" ) );
            TransUtils.beginTransaction( sdb1 );

            // 开启写事务TW3,插入R7，更新R6，删除R5
            TransUtils.beginTransaction( sdbw3 );

            BSONObject insertR7 = ( BSONObject ) JSON
                    .parse( "{_id:7,a:7,b:7}" );
            clw3.update( "{a:1}", "{$set:{b:8}}", "{'':null}" );
            clw3.delete( "{a:5}", "{'':null}" );
            clw3.insert( insertR7 );

            expList4.add( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:8}" ) );
            expList4.add( insertR7 );

            // 读事务TR3读记录
            TransUtils.queryAndCheck( cl1, null, null, "{'':null}", expList4 );
            TransUtils.commitTransaction( sdb1 );

            // 写事务TW3提交
            TransUtils.commitTransaction( sdbw3 );

        } finally {

            TransUtils.commitTransaction( sdb1 );
            TransUtils.commitTransaction( sdbw1 );
            TransUtils.commitTransaction( sdbw2 );
            TransUtils.commitTransaction( sdbw3 );

            if ( !sdb1.isClosed() ) {
                sdb1.close();
            }

            if ( !sdbw1.isClosed() ) {
                sdbw1.close();
            }

            if ( !sdbw2.isClosed() ) {
                sdbw2.close();
            }
            if ( !sdbw3.isClosed() ) {
                sdbw3.close();
            }
        }

    }

    @AfterClass
    public void tearDown() {
        TransUtils.commitTransaction( sdb );

        CollectionSpace cs = sdb.getCollectionSpace( csName );

        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }
}
