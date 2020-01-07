package com.sequoiadb.crud;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import static org.testng.Assert.*;

/**
 * Created by laojingtang on 18-1-4.
 */
public class Insert11419 extends SdbTestBase {
    private Sequoiadb db = null;
    private static final String CLNAME = Insert11419.class.getSimpleName();
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
     * 1.插入100条记录 2.并发插入新数据，并同时更新步骤1中插入的数据，待操作完成后，检查插入及更新的记录
     */
    @Test
    public void test() {
        for ( int i = 0; i < 100; i++ ) {
            dbcl.insert( new BasicBSONObject( "a", 0 ) );
        }

        SdbThreadBase insert = new SdbThreadBase() {
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( CLNAME );
                    for ( int i = 0; i < 100; i++ ) {
                        cl.insert( new BasicBSONObject( "b", i ) );
                    }
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        SdbThreadBase update = new SdbThreadBase() {
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( CLNAME );
                    for ( int i = 0; i < 10; i++ ) {
                        cl.update(
                                new BasicBSONObject( "a",
                                        new BasicBSONObject( "$isnull", 0 ) ),
                                ( BSONObject ) JSON.parse( "{$inc:{a:1}}" ),
                                new BasicBSONObject() );
                    }
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        insert.start( 20 );
        update.start( 20 );

        assertTrue( insert.isSuccess(), insert.getErrorMsg() );
        assertTrue( update.isSuccess(), update.getErrorMsg() );

        DBCursor cursor = dbcl.query();
        int a_count = 0;
        int b_count = 0;
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            if ( obj.containsField( "a" ) ) {
                int a = ( Integer ) obj.get( "a" );
                assertEquals( a, 200, obj.toString() );
                a_count++;
            } else if ( obj.containsField( "b" ) ) {
                b_count++;
            }
        }
        assertEquals( a_count, 100 );
        assertEquals( b_count, 2000 );
    }
}
