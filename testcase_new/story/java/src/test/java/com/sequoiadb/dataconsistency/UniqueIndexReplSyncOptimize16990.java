package com.sequoiadb.dataconsistency;

import java.util.ArrayList;
import java.util.Iterator;

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
 * @FileName CreateUniqueIndexsAndDataOpr16990.java
 * @content create multiple unique Indexes, concurrent execution of
 *          insert/update/delete operations, than check dataConsistency.
 * @testlink seqDB-16990
 * @author wuyan
 * @Date 2018.12.28
 * @version 1.00
 */
public class UniqueIndexReplSyncOptimize16990 extends SdbTestBase {

    private String clName = "dataConsistency16990";
    private Sequoiadb sdb = null;
    private String groupName = "";
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;
    private ArrayList< BSONObject > insertRecords = null;
    private int insertNums = 50000;
    private int updateNums = 20000;
    private int deleteNums = 20000;

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

        dbcl.createIndex( "testc", "{str:1,no:1}", true, false );
        dbcl.createIndex( "testg", "{str:-1,order:1,no:-1}", true, false );

        insertRecords = DataConsistencyUtil.insertDatas( dbcl, insertNums, 0 );
    }

    @Test
    public void test() throws Exception {
        InsertThread insertThread = new InsertThread();
        UpdateThread updateThread = new UpdateThread();
        DeleteThread deleteThread = new DeleteThread();
        insertThread.start();
        updateThread.start();
        deleteThread.start();
        Assert.assertTrue( insertThread.isSuccess(),
                insertThread.getErrorMsg() );
        Assert.assertTrue( updateThread.isSuccess(),
                updateThread.getErrorMsg() );
        Assert.assertTrue( deleteThread.isSuccess(),
                deleteThread.getErrorMsg() );

        updateExpDatas();
        DataConsistencyUtil.checkDataConsistency( sdb, SdbTestBase.csName,
                clName, insertRecords, "" );
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

    public class UpdateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                String modifier = "{ $set: { 'str': 'testdataconsistency16990' } }";
                int updateBeginNo = insertNums - updateNums;
                String matcher = "{ '$and': [ { 'inta': { '$gte': "
                        + updateBeginNo + "} }, "
                        + "				{ 'inta': { '$lt': " + insertNums
                        + "} } ] }";
                dbcl.update( matcher, modifier, "" );
            }
        }
    }

    public class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                // insert 2W records again from 5W.
                ArrayList< BSONObject > curInsertRecords = DataConsistencyUtil
                        .insertDatas( dbcl, 20000, 50000 );
                insertRecords.addAll( curInsertRecords );
            }
        }
    }

    public class DeleteThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                dbcl.delete(
                        "{ '$and': [ { 'inta': { '$gte': 0 } }, { 'inta': { '$lt': "
                                + deleteNums + " } } ] }" );
            }
        }
    }

    private void updateExpDatas() {
        Iterator< BSONObject > it = insertRecords.iterator();
        while ( it.hasNext() ) {
            BSONObject object = ( BSONObject ) it.next();
            int value = ( int ) object.get( "no" );
            if ( value >= 30000 && value < 50000 ) {
                object.put( "str", "testdataconsistency16990" );
            } else if ( value >= 0 && value < 20000 ) {
                it.remove();
            }
        }
    }
}
