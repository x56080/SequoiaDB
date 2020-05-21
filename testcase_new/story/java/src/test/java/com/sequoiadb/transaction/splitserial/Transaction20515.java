package com.sequoiadb.transaction.splitserial;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20515:存在事务操作，事务外切分卡住，事务提交后，读记录符合RR隔离级别
 * @date 2020-1-29
 * @date updated 2020-05-18 //跟进问题单5432
 * @author Lena
 * 
 */
@Test(groups = "rr")

public class Transaction20515 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private Sequoiadb tr1 = null;
    private Sequoiadb sdbw1 = null;
    private Sequoiadb tr2 = null;
    private Sequoiadb tr3 = null;
    private Sequoiadb sdbw2 = null;

    private DBCollection cl = null;
    private DBCollection tr1cl1 = null;
    private DBCollection clw1 = null;
    private DBCollection tr2cl2 = null;
    private DBCollection tr3cl3 = null;
    private DBCollection clw2 = null;

    private CollectionSpace cs;

    private String clName = "cl20515";
    private String srcGroup;
    private String desGroup;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > expList2 = new ArrayList< BSONObject >();
    private List< BSONObject > expList3 = new ArrayList< BSONObject >();

    private boolean flag;

    @BeforeClass
    public void setUp() {

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone!" );
        }

        List< String > groupsNames = CommLib.getDataGroupNames( sdb );
        if ( groupsNames.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }
        srcGroup = groupsNames.get( 0 );
        desGroup = groupsNames.get( 1 );

        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'a':1},ShardingType:'range',Group:'"
                                + srcGroup + "'}" ) );
        expList = insertData( cl );
    }

    @Test
    public void test() throws Exception {

        // 开启读事务TR1
        tr1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        tr1cl1 = tr1.getCollectionSpace( csName ).getCollection( clName );
        tr1.beginTransaction();

        // TR1读事务读记录
        TransUtils.queryAndCheck( tr1cl1, null, "{_id:1}", null, expList );

        // 开启写事务TW1更新记录
        sdbw1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        clw1 = sdbw1.getCollectionSpace( csName ).getCollection( clName );
        sdbw1.beginTransaction();

        BSONObject insertR3 = ( BSONObject ) JSON.parse( "{_id:6,a:6,b:6}" );
        clw1.update( "{a:1}", "{$set:{b:4}}", null );
        clw1.delete( "{_id:2}", null );
        clw1.insert( insertR3 );

        expList2.add( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:4}" ) );
        expList2.add( ( BSONObject ) JSON.parse( "{_id:3, a:3, b:3}" ) );
        expList2.add( ( BSONObject ) JSON.parse( "{_id:4, a:4, b:4}" ) );
        expList2.add( ( BSONObject ) JSON.parse( "{_id:5, a:5, b:5}" ) );
        expList2.add( insertR3 );

        // TW1写事务读记录
        TransUtils.queryAndCheck( clw1, null, "{_id:1}", null, expList2 );

        // TW1写事务未提交，切分
        Split split = new Split();
        split.start();

        // 等待一段时间
        try {
            Thread.sleep( 5000 );
        } catch ( Exception e ) {
            System.out.println( "Exception in Thread sleep." );
        }

        // 切分阻塞， 检验切分任务是否存在
        Assert.assertEquals(
                TransUtils.getSplitTaskStatus( sdb, cl.getFullName() ), 1 );

        // 开启读事务TR2
        tr2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        tr2cl2 = tr2.getCollectionSpace( csName ).getCollection( clName );
        tr2.beginTransaction();

        // TR2读事务读记录
        TransUtils.queryAndCheck( tr2cl2, null, "{_id:1}", null, expList );

        // 提交写事务TW1
        sdbw1.commit();

        // 等待切分任务返回
        int timeout = 0;
        while ( !flag ) {
            Thread.sleep( 1000 );
            timeout++;
            if ( timeout > 15 ) {
                throw new BaseException( 1000, "wait split timeout." );
            }
        }

        // 切分返回， 检验切分任务不存在
        DBCursor cr1 = sdb.listTasks(
                new BasicBSONObject( "Name", cl.getFullName() ), null, null,
                null );
        Assert.assertFalse( cr1.hasNext() );
        cr1.close();

        // TW1读事务读记录
        TransUtils.queryAndCheck( clw1, null, "{_id:1}", null, expList2 );

        // TR1读事务返回信息
        try {
            tr1cl1.query();
            Assert.fail( "successful query is not correct, should throw -349" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -349, e.getMessage() );
        } finally {
            tr1.rollback();
        }

        // TR2 读事务返回信息
        try {
            tr2cl2.query();
            Assert.fail( "successful query is not correct, should throw -349" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -349, e.getMessage() );
        } finally {
            tr2.rollback();
        }

        // 开启读事务TR3
        tr3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        tr3cl3 = tr3.getCollectionSpace( csName ).getCollection( clName );
        tr3.beginTransaction();

        // TR3读事务读记录
        TransUtils.queryAndCheck( tr3cl3, null, "{_id:1}", null, expList2 );

        // 切分完成后，开启写事务TW2
        sdbw2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        clw2 = sdbw2.getCollectionSpace( csName ).getCollection( clName );
        sdbw2.beginTransaction();

        // TW2写事务读记录
        TransUtils.queryAndCheck( clw2, null, "{_id:1}", null, expList2 );

        // TW2写事务更新记录
        expList3.addAll( expList2 );
        clw2.delete( "{_id:3}", null );
        expList3.remove( ( BSONObject ) JSON.parse( "{_id:3, a:3, b:3}" ) );

        // TW2写事务读记录
        TransUtils.queryAndCheck( clw2, null, "{_id:1}", null, expList3 );

    }

    // 添加5条记录
    private List< BSONObject > insertData( DBCollection cl ) {
        List< BSONObject > insertedData = new ArrayList< BSONObject >();
        for ( int i = 1; i < 6; i++ ) {
            BSONObject obj = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ",a:" + i + ",b:" + i + "}" );
            insertedData.add( obj );
        }
        cl.insert( insertedData );
        return insertedData;
    }

    @AfterClass
    public void tearDown() {

        tr1.commit();
        tr2.commit();
        tr3.commit();
        sdbw1.commit();
        sdbw2.commit();

        cs = sdb.getCollectionSpace( csName );
        cs.dropCollection( clName );

        if ( !sdb.isClosed() ) {
            sdb.close();
        }

        if ( !tr1.isClosed() ) {
            tr1.close();
        }

        if ( !tr2.isClosed() ) {
            tr2.close();
        }

        if ( !tr3.isClosed() ) {
            tr3.close();
        }

        if ( !sdbw1.isClosed() ) {
            sdbw1.close();
        }

        if ( !sdbw2.isClosed() ) {
            sdbw2.close();
        }

    }

    public class Split extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {

            Sequoiadb dbsplit1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {

                DBCollection clsplit = dbsplit1.getCollectionSpace( csName )
                        .getCollection( clName );
                clsplit.split( srcGroup, desGroup, 50 );
                flag = true;

            } finally {
                dbsplit1.close();
            }
        }
    }

}
