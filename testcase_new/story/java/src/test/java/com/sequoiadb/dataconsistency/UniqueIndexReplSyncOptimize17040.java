package com.sequoiadb.dataconsistency;

import java.util.ArrayList;
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
 * @FileName CreateUniqueIndexsAndUpdate17040.java
 * @content create multiple unique Indexes, concurrent execution of update
 *          "_id", than check dataConsistency.
 * @testlink seqDB-17040
 * @author wuyan
 * @Date 2019.1.2
 * @version 1.00
 */
public class UniqueIndexReplSyncOptimize17040 extends SdbTestBase {

    private String clName = "dataConsistency17040";
    private Sequoiadb sdb = null;
    private String groupName = "";
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;
    private ArrayList< BSONObject > expRecords = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase" );
        }

        groupName = DataConsistencyUtil.getGroupName( sdb );
        if ( DataConsistencyUtil.isOneNodeInGroup( sdb, groupName ) ) {
            throw new SkipException( "one node in group skip testcase" );
        }
        String options = "{Group:'" + groupName + "'}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        dbcl = DataConsistencyUtil.createCL( cs, clName, options );

        DataConsistencyUtil.createUnquieIndexes( cs, clName );
        expRecords = DataConsistencyUtil.insertDatas( dbcl, 50000, 0 );
    }

    @Test
    public void test() throws Exception {
        List< UpdateThread > UpdateThreads = new ArrayList<>( 10 );
        int beginNo = 0;
        int endNo = 5000;
        for ( int i = 0; i < 10; i++ ) {
            UpdateThreads.add( new UpdateThread( beginNo, endNo ) );
            beginNo = endNo;
            endNo = beginNo + 5000;
        }
        for ( UpdateThread updateThread : UpdateThreads ) {
            updateThread.start();
        }
        for ( UpdateThread updateThread : UpdateThreads ) {
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );
        }

        updateExpDatas();
        String matcherCount = "{'str':'testupdate_idfield17040'}";
        DataConsistencyUtil.checkDataConsistency( sdb, SdbTestBase.csName,
                clName, expRecords, matcherCount );
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    public class UpdateThread extends SdbThreadBase {
        private int beginNo;
        private int endNo;

        public UpdateThread( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                for ( int i = beginNo; i < endNo; i++ ) {
                    String idValue = "test_id_" + i;
                    String modifier = "{ $set: { '_id': '" + idValue
                            + "','str':'testupdate_idfield17040'} }";
                    String matcher = "{ 'no': " + i + "}";
                    dbcl.update( matcher, modifier, "" );
                }
            }
        }
    }

    // update the same range of elements in the expected list.
    private void updateExpDatas() {
        int count = 0;
        for ( BSONObject object : expRecords ) {
            object.put( "_id", "test_id_" + count );
            object.put( "str", "testupdate_idfield17040" );
            count++;
        }
    }
}
