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
public class CreateUniqueIndexsAndDelete17000 extends SdbTestBase {

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
        // insert 2W records.
        DataConsistencyUtil.insertDatas( dbcl, 200000 );
    }

    @Test
    public void test() {
        // delete 2W records per batch.
        List< DeleteThread > deleteThreads = new ArrayList<>( 10 );
        int beginNo = 0;
        int endNo = 20000;
        for ( int i = 0; i < 10; i++ ) {
            deleteThreads.add( new DeleteThread( beginNo, endNo ) );
            beginNo = endNo;
            endNo = beginNo + 20000;
        }
        for ( DeleteThread deleteThread : deleteThreads ) {
            deleteThread.start();
        }
        for ( DeleteThread deleteThread : deleteThreads ) {
            Assert.assertTrue( deleteThread.isSuccess(),
                    deleteThread.getErrorMsg() );
        }

        ArrayList< BSONObject > expRecords = new ArrayList<>();
        DataConsistencyUtil.checkDataContent( dbcl, expRecords );
        DataConsistencyUtil.checkDataConsistency( sdb, groupName,
                SdbTestBase.csName, clName, expRecords );
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

    public class DeleteThread extends SdbThreadBase {
        private int beginNo;
        private int endNo;

        public DeleteThread( int beginNo, int endNo ) {
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
                String matcher = "{ '$and': [ { 'inta': { '$gte': " + beginNo
                        + "} }," + " { 'inta': { '$lt': " + endNo + " } } ] }";
                dbcl.delete( matcher );
            } finally {
                db.disconnect();
            }
        }
    }

}
