package com.sequoiadb.rename;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description RenameCL_16090.java 并发删除索引操作和修改cl名
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCL_16090_1 extends SdbTestBase {

    private String csName = "renameCS_16090_1";
    private String clName = "rename_CL_16090_1";
    private String newCLName = "rename_CL_16090_1_new";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private int recordNum = 1000;
    private String indexNameA = "index_16090A_1";
    private String indexNameB = "index_16090B_1";
    private int dropTimes = 10;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName );
        for ( int i = 0; i < 10; i++ ) {
            cl.createIndex( indexNameA + "_" + i,
                    new BasicBSONObject( "a" + i, 1 ), false, false );
            cl.createIndex( indexNameB + "_" + i,
                    new BasicBSONObject( "b" + i, 1 ), false, false );
        }
        RenameUtil.insertData( cl, recordNum );
    }

    @Test
    public void test() {
        RenameCLThread renameCLThread = new RenameCLThread();
        DropIndexThread dropThread = new DropIndexThread();

        renameCLThread.start();
        dropThread.start();

        boolean rename = renameCLThread.isSuccess();
        boolean drop = dropThread.isSuccess();
        Assert.assertTrue( rename, renameCLThread.getErrorMsg() );

        if ( !drop ) {
            Integer[] errnos = { -23 };
            BaseException error = ( BaseException ) dropThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnos ).contains( error.getErrorCode() ) ) {
                Assert.fail( dropThread.getErrorMsg() );
            }
        }
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
            checkDropIndex( db, csName, newCLName, drop );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCS( sdb, csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                cs.renameCollection( clName, newCLName );
            }
        }
    }

    private class DropIndexThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 10; i++ ) {
                    cl.dropIndex( indexNameB + "_" + i );
                    dropTimes--;
                }
            }
        }
    }

    private void checkDropIndex( Sequoiadb db, String csName, String clName,
            boolean success ) {
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clName );
        DBCursor cur = cl.getIndexes();
        List< String > indexNames = new ArrayList< String >();
        int indexAnum = 0;
        try {
            while ( cur.hasNext() ) {
                BSONObject obj = cur.getNext();
                BSONObject indexInfo = ( BSONObject ) obj.get( "IndexDef" );
                String name = ( String ) indexInfo.get( "name" );
                indexNames.add( name );
                if ( name.indexOf( indexNameA ) != -1 ) {
                    indexAnum++;
                }
            }
        } finally {
            if ( cur != null ) {
                cur.close();
            }
        }
        Assert.assertEquals( indexAnum, 10, "check indexA num" );

        if ( success ) {
            for ( int i = 0; i < indexNames.size(); i++ ) {
                if ( indexNames.get( i ).indexOf( indexNameB ) != -1 ) {
                    Assert.fail(
                            "drop all indexB success, indexB should not exist: "
                                    + indexNames.get( i ) );
                }
            }
        } else {
            int leftNum = 0;
            for ( int i = 0; i < indexNames.size(); i++ ) {
                if ( indexNames.get( i ).indexOf( indexNameB ) != -1 ) {
                    leftNum++;
                }
            }
            if ( leftNum < dropTimes - 1 ) {
                Assert.fail( "check indexB num error, exp: " + dropTimes
                        + "act: " + leftNum );
            }
        }
    }

}
