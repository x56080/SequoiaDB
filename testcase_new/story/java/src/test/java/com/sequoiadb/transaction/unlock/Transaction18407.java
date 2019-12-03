package com.sequoiadb.transaction.unlock;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-18407: 事务1加u锁后，事务2加u锁超时，事务3加x锁继续等锁
 * @author yinzhen
 * @date 2019-6-11
 *
 */
@Test(groups = { "ru", "rc", "rs" })
public class Transaction18407 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private String clName = "cl18407";
    private String idxName = "textIndex18407";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .createCollection( clName );
        cl.createIndex( idxName, "{a:1}", false, false );
        cl.insert( "{_id:1, a:1, b:1}" );
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
        if ( db3 != null ) {
            db3.commit();
            db3.close();
        }
        if ( sdb != null ) {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
        }
    }

    @Test
    public void test() throws InterruptedException {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( clName );

        // 开启事务1，select for update R1
        db1.beginTransaction();
        DBCursor cursor = cl1.query( "{a:1}", "", "", "{'':'" + idxName + "'}",
                DBQuery.FLG_QUERY_FOR_UPDATE );
        TransUtils.getReadActList( cursor );

        // 开启事务2，select for update R1
        db2.beginTransaction();
        CL2Query th2 = new CL2Query();
        th2.start();
        Assert.assertTrue( th2.matchBlockingMethod( DBCursor.class.getName(),
                "hasNext" ) );

        Thread.sleep( TransUtils.delayTime );

        // 开启事务3，更新记录R1为R2
        db3.beginTransaction();
        CL3Update th3 = new CL3Update();
        th3.start();
        Assert.assertTrue( th3.matchBlockingMethod(
                DBCollection.class.getName(), "update" ) );

        // 待事务2等锁超时后，提交事务1，事务3返回更新成功，再次开启事务，执行查询，检查结果
        Assert.assertFalse(
                th2.isSuccess() || ( int ) th2.getExecResult() != -13,
                th2.getErrorMsg() );
        db1.commit();
        Assert.assertTrue( th3.isSuccess(), th3.getErrorMsg() );
        db3.commit();
        db1.beginTransaction();
        cursor = cl1.query();
        List< BSONObject > actList = TransUtils.getReadActList( cursor );
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" );
        Assert.assertTrue(
                actList.size() == 1 && record.equals( actList.get( 0 ) ),
                "actList: " + actList );
    }

    private class CL2Query extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try {
                DBCollection cl2 = db2.getCollectionSpace( csName )
                        .getCollection( clName );
                DBCursor cursor = cl2.query( "{a:1}", "", "",
                        "{'':'" + idxName + "'}",
                        DBQuery.FLG_QUERY_FOR_UPDATE );
                TransUtils.getReadActList( cursor );
            } catch ( BaseException e ) {
                setExecResult( e.getErrorCode() );
                throw e;
            }
        }
    }

    private class CL3Update extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            DBCollection cl3 = db3.getCollectionSpace( csName )
                    .getCollection( clName );
            cl3.update( "{a:1}", "{$set:{a:2}}", "{'':'" + idxName + "'}" );
        }
    }
}
