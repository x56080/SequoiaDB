package com.sequoiadb.transaction.rr;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

import java.util.Date;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-25067:RR隔离级别指定SDB_FLG_QUERY_FOR_SHARE走索引扫描
 * @Author liuli
 * @Date 2022.02.11
 * @UpdateAuthor liuli
 * @UpdateDate 2022.02.11
 * @version 1.10
 */
@Test(groups = "rr")
public class Transaction25067 extends SdbTestBase {
    private Sequoiadb db;
    private String csName = "cs_25067";
    private String clName = "cl_25067";
    private String indexName = "index_25067";
    private boolean queryBlockingUseFlags = true;
    private boolean runSuccess = false;
    private final static Object queryObj = new Object();
    private final static Object updateObj = new Object();
    private final static AtomicInteger count = new AtomicInteger( 0 );

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }

        if ( db.isCollectionSpaceExist( csName ) ) {
            db.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = db.createCollectionSpace( csName );
        DBCollection dbcl = dbcs.createCollection( clName );
        for ( int i = 0; i < 20; i++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "_id", i );
            record.put( "a", i );
            dbcl.insert( record );
        }
        BasicBSONObject indexKeys = new BasicBSONObject();
        indexKeys.put( "a", 1 );
        dbcl.createIndex( indexName, indexKeys, null );
    }

    @Test
    public void test() {
        QueryAndFlags query = new QueryAndFlags();
        Update update = new Update();
        query.start();
        update.start();
        Assert.assertTrue( query.isSuccess(), query.getErrorMsg() );
        Assert.assertTrue( update.isSuccess(), update.getErrorMsg() );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( db.isCollectionSpaceExist( csName ) ) {
                    db.dropCollectionSpace( csName );
                }
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    public class QueryAndFlags extends SdbThreadBase {

        @Override
        public void exec() throws BaseException, InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCursor cursor;
                db.beginTransaction();
                System.out.println( "Transaction25067 1、begin trans t1" );
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BasicBSONObject matcher = new BasicBSONObject();
                matcher.put( "a", 1 );

                BasicBSONObject hint = new BasicBSONObject();
                hint.put( "", indexName );

                // 事务t1不指定flags读取一条数据
                System.out.println( "Transaction25067 2、query one data" );
                cursor = dbcl.query( matcher, null, null, hint );
                while ( cursor.hasNext() ) {
                    BSONObject record = cursor.getNext();
                    Assert.assertEquals( record,
                            new BasicBSONObject( "a", 1 ).append( "_id", 1 ) );
                }
                cursor.close();

                // count < 2则等待更新线程执行完成后再往下执行
                count.incrementAndGet();
                synchronized ( queryObj ) {
                    if ( count.get() < 2 ) {
                        queryObj.wait();
                    }
                }

                // 更新数据后事务t1不指定flags读取一条数据
                System.out.println( "Transaction25067 4、query not used flags" );
                cursor = dbcl.query( matcher, null, null, hint );
                while ( cursor.hasNext() ) {
                    BSONObject record = cursor.getNext();
                    Assert.assertEquals( record,
                            new BasicBSONObject( "a", 1 ).append( "_id", 1 ) );
                }
                cursor.close();

                // 准备使用flags查询时唤醒更新线程
                count.incrementAndGet();
                synchronized ( updateObj ) {
                    if ( count.get() == 3 ) {
                        updateObj.notify();
                    }
                }

                System.out.println(
                        "Transaction25067 5、query with flags begin -- "
                                + new Date() );
                // 更新数据后事务t1指定flags为FLG_QUERY_FOR_SHARE，读取一条数据
                cursor = dbcl.query( matcher, null, null, hint,
                        DBQuery.FLG_QUERY_FOR_SHARE );
                while ( cursor.hasNext() ) {
                    // 查询数据后遍历游标时卡住
                    BSONObject record = cursor.getNext();
                    queryBlockingUseFlags = false;
                    Assert.assertEquals( record, new BasicBSONObject( "a", 1 )
                            .append( "b", 1 ).append( "_id", 1 ) );
                    System.out.println(
                            "Transaction25067 7、query with flags end -- "
                                    + new Date() );
                }
                cursor.close();

                // 事务t2提交后再次读取数据
                System.out.println(
                        "Transaction25067 8、query not used flags again" );
                cursor = dbcl.query( matcher, null, null, hint );
                while ( cursor.hasNext() ) {
                    BSONObject record = cursor.getNext();
                    Assert.assertEquals( record,
                            new BasicBSONObject( "a", 1 ).append( "_id", 1 ) );
                }
                cursor.close();

                db.commit();
            } catch ( BaseException e ) {
                System.out.println( "Transaction25067 e ------- " + e );
                // count增加到大于3，并将更新线程唤醒防止卡住
                count.addAndGet( 3 );
                updateObj.notify();
                throw e;
            }
        }
    }

    public class Update extends SdbThreadBase {

        @Override
        public void exec() throws BaseException, InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.beginTransaction();
                System.out.println( "Transaction25067 1、begin trans t2" );
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BasicBSONObject modifier = new BasicBSONObject();
                modifier.put( "$set", new BasicBSONObject( "b", 1 ) );

                System.out.println( "Transaction25067 3、update" );
                dbcl.update( null, modifier, null );

                // count = 2 更新完成后唤醒查询线程
                count.incrementAndGet();
                synchronized ( queryObj ) {
                    if ( count.get() == 2 ) {
                        queryObj.notify();
                    }
                }

                // 未指定flags查询时，更新线程进入等待状态
                synchronized ( updateObj ) {
                    if ( count.get() < 3 ) {
                        updateObj.wait();
                    }
                }

                // 等待5s，确认使用flags卡住至少5s
                Thread.sleep( 5000 );
                // 确认使用flags查询卡住
                Assert.assertTrue( queryBlockingUseFlags );

                // 提交事务
                db.commit();
                System.out.println( "Transaction25067 6、commit trans t2" );
            } catch ( BaseException e ) {
                System.out.println( "Transaction25067 e ------- " + e );
                // count增加到大于3，并将查询线程唤醒防止卡住
                count.addAndGet( 3 );
                queryObj.notify();
                throw e;
            }
        }
    }
}
