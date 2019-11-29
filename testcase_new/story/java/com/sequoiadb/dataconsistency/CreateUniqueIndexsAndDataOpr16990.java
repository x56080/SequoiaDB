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
 * @FileName CreateUniqueIndexsAndDataOpr16990.java
 * @content create multiple unique Indexes, concurrent execution of
 *          insert/update/delete operations, than check dataConsistency.
 * @testlink seqDB-16990
 * @author wuyan
 * @Date 2018.12.28
 * @version 1.00
 */
public class CreateUniqueIndexsAndDataOpr16990 extends SdbTestBase {

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

        insertRecords = DataConsistencyUtil.insertDatas( dbcl, insertNums );
    }

    @Test
    public void test() {
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
        DataConsistencyUtil.checkDataContent( dbcl, insertRecords );
        DataConsistencyUtil.checkDataConsistency( sdb, groupName,
                SdbTestBase.csName, clName, insertRecords );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }

        } finally {
            if ( sdb != null )
                sdb.disconnect();
        }
    }

    public class UpdateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                String modifier = "{ $set: { 'str': 'testdataconsistency16990' } }";
                int updateBeginNo = insertNums - updateNums;
                String matcher = "{ '$and': [ { 'inta': { '$gte': "
                        + updateBeginNo + "} }, "
                        + "				{ 'inta': { '$lt': " + insertNums
                        + "} } ] }";
                dbcl.update( matcher, modifier, "" );
            } finally {
                db.disconnect();
            }
        }
    }

    public class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                // insert 2W records again from 5W.
                ArrayList< BSONObject > curInsertRecords = DataConsistencyUtil
                        .insertDatas( dbcl, 20000, 50000 );
                insertRecords.addAll( curInsertRecords );
            } finally {
                db.disconnect();
            }
        }
    }

    public class DeleteThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                dbcl.delete(
                        "{ '$and': [ { 'inta': { '$gte': 0 } }, { 'inta': { '$lt': "
                                + deleteNums + " } } ] }" );
            } finally {
                db.disconnect();
            }
        }
    }

    private void updateExpDatas() {
        ArrayList< BSONObject > deleteRecords = new ArrayList< BSONObject >();
        for ( BSONObject object : insertRecords ) {
            int value = ( int ) object.get( "no" );
            // update the same elements in the expected list.
            if ( value >= 30000 && value < 50000 ) {
                object.put( "str", "testdataconsistency16990" );
            }
            if ( value >= 0 && value < 20000 ) {
                deleteRecords.add( object );

            }
        }

        // delete the same elements in the expected list.
        for ( int i = 0; i < insertRecords.size(); i++ ) {
            Object value = ( int ) insertRecords.get( i ).get( "no" );
            for ( int j = 0; j < deleteRecords.size(); j++ ) {
                Object deleteValue = deleteRecords.get( j ).get( "no" );
                if ( value.equals( deleteValue ) ) {
                    insertRecords.remove( i );
                    i--;
                }
            }
        }
    }
}
