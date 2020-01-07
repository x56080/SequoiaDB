package com.sequoiadb.crud;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * Created by laojingtang on 18-1-4.
 */
public class Insert1542 extends SdbTestBase {
    private Sequoiadb db = null;
    private static final String CLNAME = Insert1542.class.getSimpleName();
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
            CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
            cs.dropCollection( CLNAME );
            db.disconnect();
        }
    }

    /**
     * 1.插入一条记录 2.循环更新该记录10万次 3.在更新操作未结束前删除该记录
     */
    @Test
    public void test() throws InterruptedException {
        dbcl.insert( new BasicBSONObject( "a", 1 ) );

        SdbThreadBase update = new SdbThreadBase() {
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( CLNAME );
                    for ( int i = 0; i < 10000; i++ ) {
                        cl.update( new BasicBSONObject(),
                                ( BSONObject ) JSON.parse( "{$inc:{a:1}}" ),
                                new BasicBSONObject() );
                    }
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        SdbThreadBase remove = new SdbThreadBase() {
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( CLNAME );
                    cl.delete( new BasicBSONObject() );
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        update.start();
        Thread.sleep( 1000 );
        remove.start();

        Assert.assertTrue( update.isSuccess(), update.getErrorMsg() );
        Assert.assertTrue( remove.isSuccess(), remove.getErrorMsg() );
        int actualCount = ( int ) dbcl.getCount();
        Assert.assertEquals( actualCount, 0 );
    }
}
