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
 * @FileName CreateUniqueIndexsAndOpr16995.java
 * @content multiple cl create multiple unique Indexes, concurrent execution of
 *          insert and update. than check dataConsistency.
 * @testlink seqDB-16995
 * @author wuyan
 * @Date 2019.1.3
 * @version 1.00
 */
public class UniqueIndexReplSyncOptimize16995 extends SdbTestBase {
    @DataProvider(name = "dataProvider", parallel = true)
    public Object[][] generateData() {
        return new Object[][] {
                // the parameter : clName
                new Object[] { "dataConsistency_16995_a" },
                new Object[] { "dataConsistency_16995_b" },
                new Object[] { "dataConsistency_16995_c" }, };
    }

    private Sequoiadb sdb = null;
    private String csName = "cs_16995";
    private String groupName = "";

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

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        sdb.createCollectionSpace( csName );
    }

    // TODO 该用例测试点貌似没有意义，待确认用例是否要删除。经讨论暂时屏蔽，后续待跟用例责任人确认后处理。
    @Test(dataProvider = "dataProvider", enabled = false)
    public void test( String clName ) throws Exception {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            String options = "{Group:'" + groupName + "'}";
            CollectionSpace cs = db.getCollectionSpace( csName );
            DBCollection dbcl = DataConsistencyUtil.createCL( cs, clName,
                    options );
            DataConsistencyUtil.createUnquieIndexes( cs, clName );
            int beginInsertNums = 50000;
            ArrayList< BSONObject > expRecords = DataConsistencyUtil
                    .insertDatas( dbcl, beginInsertNums, 0 );

            // update 1w records per batch
            List< UpdateThread > UpdateThreads = new ArrayList<>( 5 );
            int beginNo = 0;
            int endNo = 10000;
            for ( int i = 0; i < 5; i++ ) {
                UpdateThreads.add( new UpdateThread( beginNo, endNo, clName ) );
                beginNo = endNo;
                endNo = beginNo + 10000;
            }
            for ( UpdateThread updateThread : UpdateThreads ) {
                updateThread.start();
            }
            InsertThread insertThread = new InsertThread( beginInsertNums,
                    clName, expRecords );
            insertThread.start();

            Assert.assertTrue( insertThread.isSuccess(),
                    insertThread.getErrorMsg() );
            for ( UpdateThread updateThread : UpdateThreads ) {
                Assert.assertTrue( updateThread.isSuccess(),
                        updateThread.getErrorMsg() );
            }

            updateExpDatas( expRecords, beginInsertNums );
            DataConsistencyUtil.checkDataConsistency( db, csName, clName,
                    expRecords, "" );
        }

    }

    @AfterClass
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
                String modifier = "{ $set: { 'str':'testupdate_field16995'} }";
                String matcher = "{ '$and': [ { 'no': { '$gte':" + beginNo
                        + " } }, " + "{ 'no': { '$lt': " + endNo + "}}]}";
                dbcl.update( matcher, modifier, "" );
            }
        }
    }

    public class InsertThread extends SdbThreadBase {
        private String clName;
        private int beginNo;
        private ArrayList< BSONObject > expRecords;

        public InsertThread( int beginNo, String clName,
                ArrayList< BSONObject > expRecords ) {
            this.clName = clName;
            this.beginNo = beginNo;
            this.expRecords = expRecords;
        }

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                List< BSONObject > expSubRecords = DataConsistencyUtil
                        .insertDatas( dbcl, 50000, beginNo );
                expRecords.addAll( expSubRecords );
            }
        }
    }

    // update the same range of elements in the expected list.
    private void updateExpDatas( ArrayList< BSONObject > expRecords,
            int updateNums ) {
        for ( BSONObject object : expRecords ) {
            int value = ( int ) object.get( "no" );
            if ( value < updateNums ) {
                object.put( "str", "testupdate_field16995" );
            } else {
                break;
            }
        }
    }
}
