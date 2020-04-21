package com.sequoiadb.transaction.metadata;

import java.util.ArrayList;
import java.util.List;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @testcase seqDB-18213:truncate与事务操作并发
 * @date 2019-4-11
 * @author yinzhen
 *
 */
public class Transaction18213B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18213B";
    private DBCollection cl = null;
    private List< String > groupNames;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( 2 > groupNames.size() ) {
            throw new SkipException( "groups less than 2" );
        }

        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{b:1}, ShardingType:'hash', AutoSplit: true}" ) );
        cl.createIndex( "idx18213", "{a:1}", false, false );
        cl.insert( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" ) );
        cl.insert( ( BSONObject ) JSON.parse( "{_id:2, a:2, b:2}" ) );
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启并发事务
        OperatorTh operatorTh = new OperatorTh();
        operatorTh.start();

        DropCLTh dropCLTh = new DropCLTh();
        dropCLTh.start();

        Assert.assertTrue( operatorTh.isSuccess(), operatorTh.getErrorMsg() );
        Assert.assertTrue( dropCLTh.isSuccess(), dropCLTh.getErrorMsg() );
    }

    private class OperatorTh extends SdbThreadBase {
        private Sequoiadb db;
        private DBCollection cl;

        private OperatorTh() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
        }

        @Override
        public void exec() throws Exception {
            try {
                db.beginTransaction();
                insertDatas( cl, 10000, 20000 );
                cl.delete( "{$and:[{a:{$gte:0}},{a:{$lt:5000}}]}",
                        "{'':'idx18213'}" );
                cl.update( "{$and:[{a:{$gte:5000}},{a:{$lt:15000}}]}",
                        "{$inc:{a:10}}", "{}'':'idx18213'" );
            } catch ( BaseException e ) {
                if ( -321 != e.getErrorCode() ) {
                    throw e;
                }
            } finally {
                db.commit();
                db.close();
            }
        }
    }

    // 在非事务内 truncate 集合
    private class DropCLTh extends SdbThreadBase {
        private Sequoiadb db;

        private DropCLTh() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        @Override
        public void exec() throws Exception {
            try {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                while ( true ) {
                    try {
                        Thread.sleep( 1000 );
                        cl.truncate();
                        break;
                    } catch ( BaseException e ) {
                        Assert.assertEquals( e.getErrorCode(), -190 );
                    }
                }
            } finally {
                db.close();
            }
        }
    }

    private void insertDatas( DBCollection cl, int startId, int endId ) {
        List< BSONObject > records = new ArrayList<>();
        for ( int i = startId; i < endId; i++ ) {
            records.add( ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:" + i + "}" ) );
        }
        cl.insert( records );
    }
}
