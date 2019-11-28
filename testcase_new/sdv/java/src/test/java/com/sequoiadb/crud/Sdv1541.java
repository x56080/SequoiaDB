package com.sequoiadb.crud;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static org.testng.Assert.assertTrue;

/**
 * Created by laojingtang on 18-1-4.
 */
public class Sdv1541 extends SdbTestBase {
    private Sequoiadb db = null;
    private static final String CLNAME = Sdv1541.class.getSimpleName();
    private DBCollection dbcl;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
        dbcl = cs.createCollection( CLNAME );
    }

    @AfterClass
    public void teardown() {
        if ( db != null ) {
            db.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( CLNAME );
            db.disconnect();
        }
    }

    /**
     * 以下操作全部并发执行，并连接到相同的coord操作，数据量达到百万级别： 1.创建100条bulkInsert线程
     * 2.创建100条find线程，指定从备节点上读 3.1条更新线程，更新多条数据 4.1条删除线程，更新多条数据 5.创建索引
     */
    @Test
    public void test() {
        dbcl.insert( Arrays.asList( genrateData( 10000 ) ) );

        ClTask insert = new ClTask() {
            @Override
            void opration( DBCollection cl ) {
                cl.insert( Arrays.asList( genrateData( 1000 ) ) );
            }
        };
        ClTask delete = new ClTask() {
            @Override
            void opration( DBCollection cl ) {
                for ( int i = 0; i < 100; i++ ) {
                    BasicBSONObject o = ( BasicBSONObject ) cl.queryOne();
                    if ( o == null )
                        continue;
                    cl.delete( new BasicBSONObject( "_id",
                            o.getObjectId( "_id" ) ) );
                }
            }
        };
        ClTask update = new ClTask() {
            @Override
            void opration( DBCollection cl ) {
                for ( int i = 0; i < 100; i++ ) {
                    BasicBSONObject o = ( BasicBSONObject ) cl.queryOne();
                    if ( o == null )
                        continue;
                    o.getObjectId( "_id" );
                    cl.queryAndUpdate(
                            new BasicBSONObject( "_id",
                                    o.getObjectId( "_id" ) ),
                            new BasicBSONObject(), new BasicBSONObject(),
                            new BasicBSONObject(),
                            new BasicBSONObject( "$inc",
                                    new BasicBSONObject( "a", 1 ) ),
                            0, 10, 0, true );
                }
            }
        };
        ClTask query = new ClTask() {
            @Override
            void opration( DBCollection cl ) {
                for ( int i = 0; i < 100; i++ ) {
                    cl.query( "", "", "", "", 0, 10 ).close();
                }
            }
        };

        ClTask indexCreate = new ClTask() {
            @Override
            void opration( DBCollection cl ) {
                cl.createIndex( "test_index", "{a:1}", false, false );
            }
        };

        List< ClTask > tasks = new ArrayList<>( 10 );
        tasks.add( insert );
        tasks.add( delete );
        tasks.add( update );
        tasks.add( query );
        tasks.add( indexCreate );

        insert.start( 10 );
        delete.start( 10 );
        update.start( 10 );
        query.start( 10 );
        indexCreate.start();

        for ( ClTask task : tasks ) {
            assertTrue( task.isSuccess(), task.getErrorMsg() );
        }
        DBCursor cur = dbcl.getIndex( "test_index" );
        assertTrue( cur.hasNext(), "can not find test_index " );
    }

    abstract class ClTask extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( getCoordURLRandom(), "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CLNAME );
                opration( cl );
            } finally {
                if ( db != null )
                    db.disconnect();
            }
        }

        abstract void opration( DBCollection cl );
    }

    private String getCoordURLRandom() {
        return SdbTestBase.coordUrl;
    }

    private BSONObject[] genrateData( int num ) {
        BSONObject[] b = new BSONObject[ num ];
        for ( int i = 0; i < num; i++ ) {
            b[ i ] = new BasicBSONObject( "a", i ).append( "b", i ).append( "c",
                    i );
        }
        return b;
    }
}
