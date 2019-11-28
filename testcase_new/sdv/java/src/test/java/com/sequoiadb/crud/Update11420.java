package com.sequoiadb.crud;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Created by laojingtang on 18-1-4.
 */
public class Update11420 extends SdbTestBase {
    private Sequoiadb db = null;
    private static final String CLNAME = Update11420.class.getSimpleName();
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
     * 1.插入2000条不同的记录 2.使用set操作符并发更新记录，set操作的字段为记录中不存在的字段，校验结果
     * 3.并发查询，指定条件为步骤2中新增的字段，并比对结果 4.使用unset操作符并发更新记录，unset操作的字段为记录中存在的字段，校验结果
     * 5.并发查询，指定条件为步骤4中已删除的字段，并比对结果
     */
    @Test
    public void test() {
        BSONObject[] bsonObjects = new BSONObject[ 2000 ];
        for ( int i = 0; i < 2000; i++ ) {
            bsonObjects[ i ] = new BasicBSONObject( "a", i );
        }
        dbcl.insert( Arrays.asList( bsonObjects ) );

        ClTask setTask = new ClTask() {
            @Override
            protected void update( DBCollection cl ) {
                cl.update( "", "{$set:{b:1}}", "", 0 );
            }
        };

        setTask.start( 20 );
        Assert.assertTrue( setTask.isSuccess(), setTask.getErrorMsg() );
        List< BSONObject > list = getAllRecord( dbcl.query() );
        for ( BSONObject object : list ) {
            int i = ( int ) object.get( "b" );
            Assert.assertEquals( i, 1, object.toString() );
        }

        ClTask unsetTask = new ClTask() {
            @Override
            protected void update( DBCollection cl ) {
                cl.update( "", "{$unset:{b:1}}", "", 0 );
            }
        };

        unsetTask.start( 20 );
        Assert.assertTrue( unsetTask.isSuccess(), unsetTask.getErrorMsg() );

        list = getAllRecord( dbcl.query() );
        for ( BSONObject object : list ) {
            Assert.assertFalse( object.containsField( "b" ) );
        }
    }

    private List< BSONObject > getAllRecord( DBCursor cur ) {
        List< BSONObject > actual = new ArrayList<>( 2000 );
        while ( cur.hasNext() ) {
            actual.add( cur.getNext() );
        }
        cur.close();
        return actual;
    }

    abstract class ClTask extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CLNAME );
                update( cl );
            } finally {
                if ( db != null )
                    db.disconnect();
            }

        }

        protected abstract void update( DBCollection cl );
    }

}
