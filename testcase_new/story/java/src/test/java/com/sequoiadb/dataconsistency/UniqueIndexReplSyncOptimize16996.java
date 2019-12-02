package com.sequoiadb.dataconsistency;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName CreateUniqueIndexsAndUpdate16996.java
 * @content multiple cl create multiple unique Indexes, concurrent execution of
 *          update "_id", than check dataConsistency.
 * @testlink seqDB-16996
 * @author wuyan
 * @Date 2019.1.2
 * @version 1.00
 */
public class UniqueIndexReplSyncOptimize16996 extends SdbTestBase {
    @DataProvider(name = "dataProvider", parallel = true)
    public Object[][] generateData() {
        return new Object[][] {
                // the parameter : clName
                new Object[] { "dataConsistency_16996_a" },
                new Object[] { "dataConsistency_16996_b" },
                new Object[] { "dataConsistency_16996_c" }, };
    }

    private Sequoiadb sdb = null;
    private String csName = "cs_16996";
    private String groupName = "";

    @BeforeClass(enabled = false)
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase" );
        }

        groupName = DataConsistencyUtil.getGroupName( sdb );
        if ( DataConsistencyUtil.isOneNodeInGroup( sdb, groupName ) ) {
            throw new SkipException( "one node in group skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        sdb.createCollectionSpace( csName );
    }

    @Test(dataProvider = "dataProvider", enabled = false)
    public void test( String clName ) throws Exception {
        Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        String options = "{Group:'" + groupName + "'}";
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        DBCollection dbcl = DataConsistencyUtil.createCL( cs, clName, options );
        DataConsistencyUtil.createUnquieIndexes( cs, clName );
        ArrayList< BSONObject > expRecords = DataConsistencyUtil
                .insertDatas( dbcl, 100000, 0 );

        List< UpdateThread > UpdateThreads = new ArrayList<>( 10 );
        int beginNo = 0;
        int endNo = 10000;
        for ( int i = 0; i < 10; i++ ) {
            UpdateThreads.add( new UpdateThread( beginNo, endNo, clName ) );
            beginNo = endNo;
            endNo = beginNo + 10000;
        }
        for ( UpdateThread updateThread : UpdateThreads ) {
            updateThread.start();
        }
        for ( UpdateThread updateThread : UpdateThreads ) {
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );
        }

        updateExpDatas( expRecords );
        String matcherCount = "{'str':'testupdate_idfield16996'}";
        DataConsistencyUtil.checkDataConsistency( sdb, csName, clName,
                expRecords, matcherCount );
    }

    @AfterClass(enabled = false)
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    public class UpdateThread extends SdbThreadBase {
        private String clName;
        private int beginNo;
        private int endNo;

        public UpdateThread( int beginNo, int endNo, String clName ) {
            this.clName = clName;
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = beginNo; i < endNo; i++ ) {
                    String idValue = "test_id_" + i;
                    String modifier = "{ $set: { '_id': '" + idValue
                            + "','str':'testupdate_idfield16996'} }";
                    String matcher = "{ 'no': " + i + "}";
                    dbcl.update( matcher, modifier, "" );
                }
            }
        }
    }

    // update the same range of elements in the expected list.
    private void updateExpDatas( ArrayList< BSONObject > expRecords ) {
        int count = 0;
        for ( BSONObject object : expRecords ) {
            object.put( "_id", "test_id_" + count );
            object.put( "str", "testupdate_idfield16996" );
            count++;
        }
    }
}
