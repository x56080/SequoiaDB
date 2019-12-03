package com.sequoiadb.rename;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:TestRenameCS16137
 * @content 修改cs名和删除索引并发
 * @author chensiqin
 * @Date 2018-10-20
 * @version 1.00
 */
public class TestRenameCS16137_2 extends SdbTestBase {

    private String csName = "cs16137_2";
    private String newCSName = "newcs16137_2";
    private String clName = "cl16137_2";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb.isCollectionSpaceExist( newCSName ) ) {
            sdb.dropCollectionSpace( newCSName );
        }
        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName );
        cl.createIndex( "index0", "{a0:1}", false, false );
        cl.createIndex( "index1", "{a1:-1}", false, false );
        cl.createIndex( "index2", "{a2:1}", false, false );
    }

    @Test
    public void test16137() {
        RenameCSThread renameCSThread = new RenameCSThread();
        DropIndexThread dropIndexThread = new DropIndexThread();
        renameCSThread.start();
        dropIndexThread.start();

        if ( renameCSThread.isSuccess() && !dropIndexThread.isSuccess() ) {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            RenameUtil.checkRenameCSResult( sdb, csName, newCSName, 1 );
            checkCLIndex( sdb, newCSName, clName, 3 );
            BaseException e = ( BaseException ) dropIndexThread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
        } else if ( renameCSThread.isSuccess()
                && dropIndexThread.isSuccess() ) {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            RenameUtil.checkRenameCSResult( sdb, csName, newCSName, 1 );
            checkCLIndex( sdb, newCSName, clName, 2 );
        } else {
            Assert.fail( "renameCSThread must success, but failed : "
                    + renameCSThread.getErrorMsg() );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            if ( sdb.isCollectionSpaceExist( newCSName ) ) {
                sdb.dropCollectionSpace( newCSName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    public void checkCLIndex( Sequoiadb db, String localCSName,
            String localCLName, int expected ) {
        CollectionSpace localCS = db.getCollectionSpace( localCSName );
        DBCollection localCL = localCS.getCollection( localCLName );
        DBCursor cur = localCL.getIndexes();
        int indexnum = 0;
        while ( cur.hasNext() ) {
            indexnum++;
            cur.getNext();
        }
        cur.close();
        Assert.assertEquals( indexnum, expected + 1 );
    }

    private class RenameCSThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                db.renameCollectionSpace( csName, newCSName );
            } finally {
                db.close();
            }
        }
    }

    private class DropIndexThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                CollectionSpace localcs = db.getCollectionSpace( csName );
                DBCollection localcl = localcs.getCollection( clName );
                localcl.dropIndex( "index0" );
            } finally {
                db.close();
            }
        }
    }

}