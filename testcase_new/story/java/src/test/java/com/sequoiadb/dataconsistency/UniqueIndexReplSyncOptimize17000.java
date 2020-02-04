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
 * @FileName CreateUniqueIndexsAndUpdate17000.java
 * @content create multiple unique Indexes, concurrent execution of delete
 *          operations, than check dataConsistency.
 * @testlink seqDB-17000
 * @author wuyan
 * @Date 2019.1.2
 * @version 1.00
 */
public class UniqueIndexReplSyncOptimize17000 extends SdbTestBase {

    private String clName = "dataConsistency17000";
    private Sequoiadb sdb = null;
    private String groupName = "";
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;

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
        // insert 2W records.
        DataConsistencyUtil.insertDatas( dbcl, 50000, 0 );
    }

    @Test
    public void test() throws Exception {
        // delete 2W records per batch.
        List< DeleteThread > deleteThreads = new ArrayList<>( 10 );
        int beginNo = 0;
        int endNo = 5000;
        for ( int i = 0; i < 10; i++ ) {
            deleteThreads.add( new DeleteThread( beginNo, endNo ) );
            beginNo = endNo;
            endNo = beginNo + 5000;
        }
        for ( DeleteThread deleteThread : deleteThreads ) {
            deleteThread.start();
        }
        for ( DeleteThread deleteThread : deleteThreads ) {
            Assert.assertTrue( deleteThread.isSuccess(),
                    deleteThread.getErrorMsg() );
        }

        ArrayList< BSONObject > expRecords = new ArrayList<>();
        DataConsistencyUtil.checkDataConsistency( sdb, SdbTestBase.csName,
                clName, expRecords, "" );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }

        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    public class DeleteThread extends SdbThreadBase {
        private int beginNo;
        private int endNo;

        public DeleteThread( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                String matcher = "{ '$and': [ { 'inta': { '$gte': " + beginNo
                        + "} }," + " { 'inta': { '$lt': " + endNo + " } } ] }";
                dbcl.delete( matcher );
            }
        }
    }

}
