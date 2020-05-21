package com.sequoiadb.transaction.splitserial;

import java.util.ArrayList;
import java.util.List;
import org.bson.BSONObject;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-22117:集合切分与其他集合事务交互
 * @date 2020-4-26 //跟进问题单5432, 添加新测试用例
 * @date 2020-05-18
 *       //这个测试用例修改预期结果为阻塞来实现自动化，目前的实现是阻塞的；待问题单5771优化后，再通过该用例来验证这个点：同一个组上，一个集合上未提交的事务，不应该阻塞另一个集合上的切分操作；
 * 
 * @author Lena
 */
@Test(groups = "rr")
public class Transaction22117 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private Sequoiadb sdbw1 = null;

    private CollectionSpace cs;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private DBCollection clw1 = null;

    private String clName = "cl22117";
    private String clName2 = "cl22117_2";
    private String srcGroup;
    private String desGroup;
    DBCursor cur;

    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > expList2 = new ArrayList< BSONObject >();

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
                ( BSONObject ) JSON.parse( "{Group:'" + srcGroup + "'} " ) );
        cl2 = cs.createCollection( clName2,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'a':1},ShardingType:'range',Group:'"
                                + srcGroup + "'}" ) );

        expList = insertData( cl );
        insertData( cl2 );
    }

    @Test
    public void test() {

        // 开启写事务TW1更新记录
        sdbw1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        clw1 = sdbw1.getCollectionSpace( csName ).getCollection( clName );
        sdbw1.beginTransaction();

        TransUtils.queryAndCheck( clw1, null, "{_id:1}", null, expList );

        BSONObject insertR3 = ( BSONObject ) JSON.parse( "{_id:7,a:7,b:7}" );
        clw1.update( "{a:1}", "{$set:{b:4}}", null );
        clw1.delete( "{_id:2}", null );
        clw1.insert( insertR3 );

        expList2.add( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:4}" ) );
        expList2.add( ( BSONObject ) JSON.parse( "{_id:3, a:3, b:3}" ) );
        expList2.add( ( BSONObject ) JSON.parse( "{_id:4, a:4, b:4}" ) );
        expList2.add( ( BSONObject ) JSON.parse( "{_id:5, a:5, b:5}" ) );
        expList2.add( ( BSONObject ) JSON.parse( "{_id:6, a:6, b:6}" ) );
        expList2.add( insertR3 );

        // TW1写事务读记录
        TransUtils.queryAndCheck( clw1, null, "{_id:1}", null, expList2 );

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
                TransUtils.getSplitTaskStatus( sdb, cl2.getFullName() ), 1 );
    }

    // 添加5条记录
    private List< BSONObject > insertData( DBCollection cl ) {
        List< BSONObject > insertedData = new ArrayList< BSONObject >();
        for ( int i = 1; i <= 6; i++ ) {
            BSONObject obj = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ",a:" + i + ",b:" + i + "}" );
            insertedData.add( obj );
        }
        cl.insert( insertedData );
        return insertedData;
    }

    @AfterClass
    public void tearDown() {
        sdbw1.commit();

        cs = sdb.getCollectionSpace( csName );
        cs.dropCollection( clName );
        cs.dropCollection( clName2 );

        if ( cur != null ) {
            cur.close();
        }

        if ( !sdb.isClosed() ) {
            sdb.close();
        }

        if ( !sdbw1.isClosed() ) {
            sdbw1.close();
        }
    }

    public class Split extends SdbThreadBase {

        @Override
        public void exec() {

            Sequoiadb dbsplit1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {

                DBCollection clsplit = dbsplit1.getCollectionSpace( csName )
                        .getCollection( clName2 );
                clsplit.split( srcGroup, desGroup, 50 );
            } finally {
                dbsplit1.close();
            }
        }
    }

}
