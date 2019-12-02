package com.sequoiadb.dataconsistency;

import java.util.ArrayList;

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
 * @FileName CreateUniqueIndexsAndDataOpr17004.java
 * @content multiple cl create unique Indexes, concurrent execution of
 *          insert/delete/update operations, eg:cl1 update "_id" field, than
 *          delete; cl2 insert ,than update "_id" field.the delete and insert
 *          data nums greater than 1w. than check dataConsistency.
 * @testlink seqDB-17004
 * @author wuyan
 * @Date 2019.1.3
 * @version 1.00
 */
public class UniqueIndexReplSyncOptimize17004a extends SdbTestBase {

    private String clName1 = "dataConsistency17004_a1";
    private String clName2 = "dataConsistency17004_a2";
    private Sequoiadb sdb = null;
    private String groupName = "";
    private CollectionSpace cs = null;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private ArrayList< BSONObject > insertRecordsInCL1 = null;
    private ArrayList< BSONObject > insertRecordsInCL2 = null;
    private int insertNums = 100000;

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
        dbcl1 = DataConsistencyUtil.createCL( cs, clName1, options );
        dbcl2 = DataConsistencyUtil.createCL( cs, clName2, options );

        DataConsistencyUtil.createUnquieIndexes( cs, clName1 );
        DataConsistencyUtil.createUnquieIndexes( cs, clName2 );
        insertRecordsInCL1 = DataConsistencyUtil.insertDatas( dbcl1, insertNums,
                0 );
    }

    @Test
    public void test() throws Exception {
        InsertAndUpdateThread insertAndUpdateThread = new InsertAndUpdateThread(
                clName2 );
        UpdateAndDeleteThread updateAndDeleteThread = new UpdateAndDeleteThread(
                clName1, insertNums );
        insertAndUpdateThread.start();
        updateAndDeleteThread.start();
        Assert.assertTrue( insertAndUpdateThread.isSuccess(),
                insertAndUpdateThread.getErrorMsg() );
        Assert.assertTrue( updateAndDeleteThread.isSuccess(),
                updateAndDeleteThread.getErrorMsg() );

        insertRecordsInCL1.clear();
        DataConsistencyUtil.checkDataConsistency( sdb, csName, clName1,
                insertRecordsInCL1, "" );

        // update the same elements in the expected list.
        updateExpDatas( insertRecordsInCL2 );
        String matcher = "{'str':'testupdate_idfield17004a2'}";
        DataConsistencyUtil.checkDataContent( dbcl2, insertRecordsInCL2,
                matcher );
        DataConsistencyUtil.checkDataConsistency( sdb, csName, clName2,
                insertRecordsInCL2, matcher );

    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName1 );
            cs.dropCollection( clName2 );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    public class UpdateAndDeleteThread extends SdbThreadBase {
        private String clName;
        private int updateNums;

        public UpdateAndDeleteThread( String clName, int updateNums ) {
            this.clName = clName;
            this.updateNums = updateNums;
        }

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                for ( int i = 0; i < updateNums; i++ ) {
                    String idValue = "test_id_" + i;
                    String modifier = "{ $set: { '_id': '" + idValue
                            + "','str':'testupdate_idfield17004a1'} }";
                    String matcher = "{ 'no': " + i + "}";
                    dbcl.update( matcher, modifier, "" );
                }
                dbcl.delete( "" );
            }
        }
    }

    public class InsertAndUpdateThread extends SdbThreadBase {
        private String clName;

        public InsertAndUpdateThread( String clName ) {
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            int insertNums = 100000;
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                insertRecordsInCL2 = DataConsistencyUtil.insertDatas( dbcl,
                        insertNums, 0 );
                for ( int i = 0; i < insertNums; i++ ) {
                    String idValue = "test_id_" + i;
                    String modifier = "{ $set: { '_id': '" + idValue
                            + "','str':'testupdate_idfield17004a2'} }";
                    String matcher = "{ 'no': " + i + "}";
                    dbcl.update( matcher, modifier, "" );
                }
            }
        }
    }

    private void updateExpDatas( ArrayList< BSONObject > insertRecords ) {
        int count = 0;
        for ( BSONObject object : insertRecords ) {
            object.put( "str", "testupdate_idfield17004a2" );
            object.put( "_id", "test_id_" + count );
            count++;
        }
    }

}
