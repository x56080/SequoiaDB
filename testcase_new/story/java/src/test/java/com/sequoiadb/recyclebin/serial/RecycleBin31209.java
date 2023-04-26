package com.sequoiadb.recyclebin.serial;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.recyclebin.RecycleBinUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import java.util.ArrayList;
import java.util.List;

/**
 * @Descreption seqDB-31209:truncate并发删除相同CL项目，备注不同
 * @Author biqin
 * @CreateDate 2023/4/21
 * @UpdateUser biqin
 * @UpdateDate 2023/4/21
 * @UpdateRemark
 * @Version 1.0
 */
@Test(groups = "recycleBin")
public class RecycleBin31209 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_31209";
    private String clName = "cl_31909";
    private String truncateComment = "truncateComment";
    private Integer MaxVersionNum = 10;
    private DBCollection dbcl = null;
    private boolean runSuccess = false;
    private List< Integer > saveResultNumber = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        sdb.getRecycleBin()
                .alter( new BasicBSONObject( "MaxVersionNum", MaxVersionNum ) );
        RecycleBinUtils.cleanRecycleBin( sdb, csName );
        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        dbcl = dbcs.createCollection( clName );
        // 写入10000数据
        int recordNum = 10000;
        RecycleBinUtils.insertData( dbcl, recordNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < 10; i++ ) {
            es.addWorker( new Truncate( i ) );
        }
        es.run();
        checkInsertResult();
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        if ( runSuccess ) {
            sdb.dropCollectionSpace( csName );
            RecycleBinUtils.cleanRecycleBin( sdb, csName );
        }
        sdb.getRecycleBin().alter( new BasicBSONObject( "MaxVersionNum", 2 ) );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private void checkInsertResult() {
        for ( Integer number : saveResultNumber ) {
            BasicBSONObject option = new BasicBSONObject( "Comment",
                    truncateComment + number );
            option.put( "OriginName", csName + "." + clName );
            DBCursor cursor = sdb.getRecycleBin().list( option, null, null );
            int count = 0;
            String comment = "";
            while ( cursor.hasNext() ) {
                BSONObject result = cursor.getNext();
                comment = ( String ) result.get( "Comment" );
                count++;
            }
            Assert.assertEquals( count, 1 );
            Assert.assertEquals( comment, truncateComment + number );
        }
    }

    private class Truncate {
        Integer number;

        private Truncate( Integer number ) {
            this.number = number;
        }

        @ExecuteOrder(step = 1)
        private void truncate() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BasicBSONObject option = new BasicBSONObject( "Comment",
                        truncateComment + number );
                dbcl.truncate( option );
                saveResultNumber.add( number );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

}
