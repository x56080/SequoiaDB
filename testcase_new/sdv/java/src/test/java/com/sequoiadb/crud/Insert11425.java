package com.sequoiadb.crud;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;
import java.util.Random;

/**
 * Created by laojingtang on 18-1-4.
 */
public class Insert11425 extends SdbTestBase {
    private Sequoiadb db = null;
    private static final String CLNAME = Insert11425.class.getSimpleName();
    private CollectionSpace cs = null;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( CLNAME );
    }

    /**
     * 1.多线程插入，同时删除cl
     */
    @Test
    public void test() throws InterruptedException {
        InsertTask insert = new InsertTask();
        insert.start( 20 );

        Thread.sleep( new Random().nextInt( 1000 ) );
        // drop cl fail is -147/-190, repeat drop cL again
        int eachSleepTime = 1000;
        int maxSleetTime = 60000;
        int alreadySleepTime = 0;
        int errorNo = 0;
        do {
            errorNo = dropCL( cs );
            Thread.sleep( eachSleepTime );
            alreadySleepTime += eachSleepTime;
            if ( alreadySleepTime > maxSleetTime )
                Assert.fail( "drop cl fail exceeds maximum waiting time:"
                        + alreadySleepTime );
        } while ( errorNo == -147 || errorNo == -190 );

        Assert.assertTrue( insert.isSuccess(), insert.getErrorMsg() );
        Assert.assertFalse( cs.isCollectionExist( CLNAME ),
                "drop CL fail!e=" + errorNo );
    }

    @AfterClass
    public void teardown() {
        if ( db != null ) {
            db.close();
        }
    }

    private int dropCL( CollectionSpace cs ) {
        int errorNo = 0;
        try {
            cs.dropCollection( CLNAME );
        } catch ( BaseException e ) {
            errorNo = e.getErrorCode();
        }
        return errorNo;
    }

    private class InsertTask extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CLNAME );
                for ( int i = 0; i < 10000; i++ ) {
                    BSONObject obj = new BasicBSONObject();
                    obj.put( "a", i );
                    obj.put( "str", "test_" + String.valueOf( i ) );
                    Date now = new Date();
                    obj.put( "date", now );
                    cl.insert( obj );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -248
                        && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    Assert.fail( "insert fail!! e=" + e.getErrorCode() );
                }
            }
        }
    }
}
