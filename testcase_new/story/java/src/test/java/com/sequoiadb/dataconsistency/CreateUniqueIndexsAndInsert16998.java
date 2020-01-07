package com.sequoiadb.dataconsistency;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName CreateUniqueIndexsAndInsert16998.java
 * @content create multiple unique Indexes, concurrent execution of insert
 *          operations, than check dataConsistency.
 * @testlink seqDB-16998
 * @author wuyan
 * @Date 2019.1.2
 * @version 1.00
 */
public class CreateUniqueIndexsAndInsert16998 extends SdbTestBase {

    private String clName = "dataConsistency16998";
    private Sequoiadb sdb = null;
    private String groupName = "";
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;
    private int recordNums = 100000;
    private List< BSONObject > expRecords = Collections
            .synchronizedList( new ArrayList< BSONObject >( recordNums ) );

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase" );
        }

        groupName = DataConsistencyUtil.getGroupName( sdb );

        String options = "{ShardingKey:{no:1},ReplSize:1,Group:'" + groupName
                + "'}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        dbcl = DataConsistencyUtil.createCL( cs, clName, options );

        dbcl.createIndex( "testa", "{no:1}", true, false );
        dbcl.createIndex( "testb", "{inta:1,no:1}", true, false );
        dbcl.createIndex( "testc", "{str:1,no:1}", true, false );
        dbcl.createIndex( "teste", "{ftest:1,no:-1}", true, false );
        dbcl.createIndex( "testf", "{ftest:-1,no:1}", true, false );
        dbcl.createIndex( "testg", "{str:-1,order:1,no:-1}", true, false );
    }

    @Test
    public void test() {
        List< InsertThread > insertThreads = new ArrayList<>( 10 );
        int beginNo = 0;
        int endNo = 20000;
        int numsPerBatch = 20000;
        for ( int i = 0; i < recordNums / numsPerBatch; i++ ) {
            insertThreads.add( new InsertThread( beginNo, endNo ) );
            beginNo = endNo;
            endNo = beginNo + 20000;
        }
        for ( InsertThread insertThread : insertThreads ) {
            insertThread.start();
        }
        for ( InsertThread insertThread : insertThreads ) {
            Assert.assertTrue( insertThread.isSuccess(),
                    insertThread.getErrorMsg() );
        }

        DataConsistencyUtil.checkDataContent( dbcl, expRecords, "" );
        DataConsistencyUtil.checkDataConsistency( sdb, groupName,
                SdbTestBase.csName, clName, expRecords, "" );
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            if ( sdb != null )
                sdb.disconnect();
        }
    }

    public class InsertThread extends SdbThreadBase {
        private int beginNo;
        private int endNo;

        public InsertThread( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                List< BSONObject > expSubRecords = DataConsistencyUtil
                        .insertDatas( dbcl, endNo - beginNo, beginNo );

                // save the number of inserted records sequentially into the
                // expRecordsList.
                // use addAll(x,list<>) must be added to list in order.
                do {
                    if ( expRecords.size() == beginNo ) {
                        expRecords.addAll( beginNo, expSubRecords );
                        break;
                    }
                } while ( true );
            } finally {
                db.disconnect();
            }
        }
    }
}
