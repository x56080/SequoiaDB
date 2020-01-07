package com.sequoiadb.index;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.sequoiadb.index.IndexUtil.*;
import static com.sequoiadb.index.IndexUtil.assertIndexCreatedCorrect;
import static org.testng.Assert.*;

/**
 * Created by laojingtang on 18-1-2.
 */
public class Index11416 extends SdbTestBase {
    final String CLNAME = Index11416.class.getSimpleName();
    private Sequoiadb db = null;
    private DBCollection dbcl;
    private List< BSONObject > bsonInserted = new ArrayList<>( 10000 );

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .createCollection( CLNAME );
        // prepare data
        for ( int i = 0; i < 10000; i++ ) {
            BSONObject obj = new BasicBSONObject().append( "a", i )
                    .append( "b", i ).append( "c", i ).append( "d", i );
            bsonInserted.add( obj );
        }
        dbcl.bulkInsert( bsonInserted, 0 );
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
     * 1、多个线程并发执行查询操作（带索引查询） 2、查询过程中并发执行创建、删除索引操作 3、检查操作结果
     */
    @Test
    public void testCreateIndexAndQuery() {
        final IndexEntity createIndex = new IndexEntity()
                .setIndexName( "index11416" )
                .setKey( new BasicBSONObject( "a", 1 ) );
        SdbThreadBase createTasks = new SdbThreadBase() {
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( Index11416.this.CLNAME );
                    cl.createIndex( createIndex.getIndexName(),
                            createIndex.getKey(), createIndex.isUnique(),
                            createIndex.isEnforced() );
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        SdbThreadBase queryTask = new QueryTask( "index11416" );
        queryTask.start( 20 );
        createTasks.start();

        assertTrue( queryTask.isSuccess(), queryTask.getErrorMsg() );
        assertTrue( createTasks.isSuccess(), createTasks.getErrorMsg() );

        // assert index info
        assertIndexCreatedCorrect( dbcl, createIndex );

        // assert index can be used
        Map< Object, BSONObject > expectRecordMap = new HashMap<>();
        for ( BSONObject obj : bsonInserted ) {
            expectRecordMap.put( obj.get( "_id" ), obj );
        }
        BSONObject matcher = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        BSONObject orderby = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject( "", createIndex.getIndexName() );
        DBCursor cur = null;
        try {
            cur = dbcl.query( matcher, selector, orderby, hint );
            while ( cur.hasNext() ) {
                BSONObject obj = cur.getNext();
                assertEquals( obj, expectRecordMap.get( obj.get( "_id" ) ) );
            }
        } finally {
            if ( cur != null )
                cur.close();
        }
    }

    @Test
    public void testRemoveIndexAndQuery() {
        dbcl.createIndex( "b_index", new BasicBSONObject( "b", 1 ), false,
                false );
        long initCount = dbcl.getCount();

        SdbThreadBase removeTask = new SdbThreadBase() {
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( Index11416.this.CLNAME );
                    cl.dropIndex( "b_index" );
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };
        SdbThreadBase queryTask = new QueryTask( "b_index" );
        removeTask.start();
        queryTask.start( 20 );

        assertTrue( removeTask.isSuccess(), removeTask.getErrorMsg() );
        assertTrue( queryTask.isSuccess(), queryTask.getErrorMsg() );
        DBCursor curor = null;
        try {
            curor = dbcl.getIndex( "b_index" );
            assertFalse( curor.hasNext(), "b_index" );
        } finally {
            if ( curor != null )
                curor.close();
        }

        long actual = 0;
        DBCursor cur = null;
        try {
            cur = dbcl.query();
            while ( cur.hasNext() ) {
                cur.getNext();
                actual++;
            }
        } finally {
            if ( cur != null )
                cur.close();
        }
        assertEquals( actual, initCount );
    }

    class QueryTask extends SdbThreadBase {

        private String indexName;

        public QueryTask( String indexName ) {
            this.indexName = indexName;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( Index11416.this.CLNAME );
                cl.query( null, null, null,
                        new BasicBSONObject( "", indexName ), 0, 10 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -10 )
                    throw e;
            } finally {
                if ( db != null )
                    db.disconnect();
            }
        }
    }
}
