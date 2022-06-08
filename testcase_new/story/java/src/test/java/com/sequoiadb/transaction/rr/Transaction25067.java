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
    private boolean beginQueryUseFlags = false;
    private boolean beginUpdata = false;
    private boolean endUpdata = false;
    private boolean updateTransCommit = false;
    private boolean runSuccess = false;
    private final static Object syncObj = new Object();

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
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BasicBSONObject matcher = new BasicBSONObject();
                matcher.put( "a", 1 );

                BasicBSONObject hint = new BasicBSONObject();
                hint.put( "", indexName );

                // 事务t1不指定flags读取一条数据
                cursor = dbcl.query( matcher, null, null, hint );
                while ( cursor.hasNext() ) {
                    BSONObject record = cursor.getNext();
                    Assert.assertEquals( record,
                            new BasicBSONObject( "a", 1 ).append( "_id", 1 ) );
                }
                cursor.close();

                beginUpdata = true;
                // 唤醒更新线程
                synchronized ( syncObj ) {
                    if ( beginUpdata ) {
                        syncObj.notifyAll();
                    } else {
                        syncObj.wait();
                    }
                }

                // 事务t2更新完数据后再查询
                synchronized ( syncObj ) {
                    if ( endUpdata ) {
                        syncObj.notifyAll();
                    } else {
                        syncObj.wait();
                    }
                }

                // 更新数据后事务t1不指定flags读取一条数据
                cursor = dbcl.query( matcher, null, null, hint );
                while ( cursor.hasNext() ) {
                    BSONObject record = cursor.getNext();
                    Assert.assertEquals( record,
                            new BasicBSONObject( "a", 1 ).append( "_id", 1 ) );
                }
                cursor.close();

                beginQueryUseFlags = true;
                // 唤醒更新线程校验flags查询卡住
                synchronized ( syncObj ) {
                    if ( beginQueryUseFlags ) {
                        syncObj.notifyAll();
                    } else {
                        syncObj.wait();
                    }
                }

                // 更新数据后事务t1指定flags为FLG_QUERY_FOR_SHARE，读取一条数据
                cursor = dbcl.query( matcher, null, null, hint,
                        DBQuery.FLG_QUERY_FOR_SHARE );
                while ( cursor.hasNext() ) {
                    // 查询数据后遍历游标时卡住
                    BSONObject record = cursor.getNext();
                    queryBlockingUseFlags = false;
                    Assert.assertEquals( record, new BasicBSONObject( "a", 1 )
                            .append( "b", 1 ).append( "_id", 1 ) );
                }
                cursor.close();

                // 事务t2提交后再次读取数据
                synchronized ( syncObj ) {
                    if ( updateTransCommit ) {
                        syncObj.notifyAll();
                    } else {
                        syncObj.wait();
                    }
                }

                cursor = dbcl.query( matcher, null, null, hint );
                while ( cursor.hasNext() ) {
                    BSONObject record = cursor.getNext();
                    Assert.assertEquals( record,
                            new BasicBSONObject( "a", 1 ).append( "_id", 1 ) );
                }
                cursor.close();

                db.commit();
            } catch ( BaseException e ) {
                System.out.println( "e ------- " + e );
                // 将所有控制变量置位true，然后唤醒线程，防止一个线程执行失败后另一个线程卡住
                beginQueryUseFlags = true;
                beginUpdata = true;
                endUpdata = true;
                updateTransCommit = true;
                synchronized ( syncObj ) {
                    syncObj.notifyAll();
                }
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
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BasicBSONObject modifier = new BasicBSONObject();
                modifier.put( "$set", new BasicBSONObject( "b", 1 ) );

                // 更新线程等待被唤醒
                synchronized ( syncObj ) {
                    if ( beginUpdata ) {
                        syncObj.notifyAll();
                    } else {
                        syncObj.wait();
                    }
                }
                dbcl.update( null, modifier, null );

                endUpdata = true;
                // 更新结束后事务t1再次开始查询数据
                synchronized ( syncObj ) {
                    if ( endUpdata ) {
                        syncObj.notifyAll();
                    } else {
                        syncObj.wait();
                    }
                }

                // 更新数据后开始使用flags查询数据再往下执行
                synchronized ( syncObj ) {
                    if ( beginQueryUseFlags ) {
                        syncObj.notifyAll();
                    } else {
                        syncObj.wait();
                    }
                }

                // 等待5s，确认使用flags卡住至少5s
                Thread.sleep( 5000 );

                // 确认使用flags查询卡住
                Assert.assertTrue( queryBlockingUseFlags );

                // 提交事务
                db.commit();

                updateTransCommit = true;
                synchronized ( syncObj ) {
                    if ( updateTransCommit ) {
                        syncObj.notifyAll();
                    } else {
                        syncObj.wait();
                    }
                }
            } catch ( BaseException e ) {
                System.out.println( "e ------- " + e );
                // 将所有控制变量置位true，然后唤醒线程，防止一个线程执行失败后另一个线程卡住
                beginQueryUseFlags = true;
                beginUpdata = true;
                endUpdata = true;
                updateTransCommit = true;
                synchronized ( syncObj ) {
                    syncObj.notifyAll();
                }
                throw e;
            }
        }
    }
}
