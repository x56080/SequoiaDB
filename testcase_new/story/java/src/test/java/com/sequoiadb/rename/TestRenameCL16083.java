package com.sequoiadb.rename;

import org.testng.Assert;
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
 * @FileName:TestRenameCL16083
 * @content 并发修改cl名和删除相同cl
 * @author chensiqin
 * @Date 2018-10-23
 * @version 1.00
 */
public class TestRenameCL16083 extends SdbTestBase {

    private String clName = "cl16083";
    private String newclName = "newcl16083";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        DBCollection cl1 = cs.createCollection( clName );
        RenameUtil.insertData( cl1, 5 );
    }

    @Test
    public void test16083() {
        RenameCLThread renameCLThread = new RenameCLThread();
        DropCLThread dropCLThread = new DropCLThread();
        renameCLThread.start();
        dropCLThread.start();

        if ( renameCLThread.isSuccess() && !dropCLThread.isSuccess() ) {
            RenameUtil.checkRenameCLResult( sdb, SdbTestBase.csName, clName,
                    newclName );
            Assert.assertEquals( cs.isCollectionExist( clName ), false );
            BaseException e = ( BaseException ) dropCLThread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() != -23 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
        } else if ( !renameCLThread.isSuccess() && dropCLThread.isSuccess() ) {
            Assert.assertEquals( cs.isCollectionExist( clName ), false );
            Assert.assertEquals( cs.isCollectionExist( newclName ), false );
            BaseException e = ( BaseException ) renameCLThread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() != -23 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
        } else if ( renameCLThread.isSuccess() && dropCLThread.isSuccess() ) {
            System.out.println( "wo dou chenggong le " );
        } else {
            Assert.fail( "renameCLThread and dropCLThread failed: "
                    + renameCLThread.getErrorMsg()
                    + dropCLThread.getErrorMsg() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCL( sdb, SdbTestBase.csName, clName );
            CommLib.clearCL( sdb, SdbTestBase.csName, newclName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                CollectionSpace localcs = db
                        .getCollectionSpace( SdbTestBase.csName );
                localcs.renameCollection( clName, newclName );

            } finally {
                db.close();
            }
        }
    }

    private class DropCLThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                CollectionSpace localcs = db
                        .getCollectionSpace( SdbTestBase.csName );
                localcs.dropCollection( clName );
            } finally {
                db.close();
            }
        }
    }
}
