package com.sequoiadb.dataconsistency;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @testlink seqDB-20248:存在多个唯一索引，插入/更新记录在备节点重放记录与其他桶产生duplicated key错误
 * @author zhaoyu
 * @Date 2019.11.11
 */
public class UniqueIndexReplSyncOptimize20248 extends SdbTestBase {

    private String clName = "cl20248";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;
    private int loopNum = 10000;
    private String groupName;
    private ArrayList< BSONObject > expRecords = new ArrayList<>();
    private BSONObject record;

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

        cs = sdb.getCollectionSpace( csName );
        dbcl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        dbcl.createIndex( "a20248", "{a:1}", true, true );
        record = ( BSONObject ) JSON.parse( "{_id:3,a:3,order:1}" );
        expRecords.add( record );
        record = ( BSONObject ) JSON
                .parse( "{_id:4,a:4,order:2,c:'insertRecord'}" );
        expRecords.add( record );
        dbcl.insert( expRecords );
    }

    @Test
    public void test() throws Exception {
        InsertThread insertThread = new InsertThread();
        UpdateThread updateThread = new UpdateThread();
        insertThread.start();
        updateThread.start();
        Assert.assertTrue( insertThread.isSuccess(),
                insertThread.getErrorMsg() );
        Assert.assertTrue( updateThread.isSuccess(),
                updateThread.getErrorMsg() );

        expRecords.clear();
        int bValue = loopNum - 1;
        record = ( BSONObject ) JSON.parse(
                "{_id:3,a:3,b:" + bValue + ",order:1, c:'incRecordLength'}" );
        expRecords.add( record );
        record = ( BSONObject ) JSON
                .parse( "{_id:4,a:4,b:" + bValue + ",order:2, c:'update'}" );
        expRecords.add( record );
        DataConsistencyUtil.checkDataConsistency( sdb, csName, clName,
                expRecords, "" );

    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

    public class InsertThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    String cValue = getRandomString();
                    cl.insert( "{_id:1,a:1,c:'" + cValue + "'}" );
                    cl.delete( "{_id:1}" );
                    cl.insert( "{_id:2,a:1,c:'" + cValue + "'}" );
                    cl.delete( "{_id:2}" );
                    // analyze会写日志，但是这个日志不会并发重放，验证并发重放转成非并发重放的正确性
                    if ( 0 == i % 1000 ) {
                        BSONObject analyzeOtions = ( BSONObject ) JSON
                                .parse( "{Collection: '" + csName + "." + clName
                                        + "' }" );
                        db.analyze( analyzeOtions );
                    }
                }

            }
        }
    }

    public class UpdateThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    cl.update( "{_id:3}", "{$set:{a:13,c:''}}", null );
                    cl.update( "{_id:3}",
                            "{$set:{a:3,b:" + i + ",c:'incRecordLength'}}",
                            null );
                    cl.update( "{_id:4}", "{$set:{a:13,c:'incRecordLength'}}",
                            null );
                    cl.update( "{_id:4}", "{$set:{a:4,b:" + i + ",c:'update'}}",
                            null );

                    if ( 0 == i % 1000 ) {
                        BSONObject analyzeOtions = ( BSONObject ) JSON
                                .parse( "{Collection: '" + csName + "." + clName
                                        + "' }" );
                        db.analyze( analyzeOtions );
                    }
                }

            }
        }
    }

    public String getRandomString() {
        int num = ( int ) ( Math.random() * 30 + 1 );
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < num; i++ ) {
            sb.append( "a" );
        }
        return sb.toString();
    }

}
