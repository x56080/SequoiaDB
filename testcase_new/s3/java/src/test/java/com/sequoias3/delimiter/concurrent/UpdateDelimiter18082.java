package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18082: current delimiter status is not normal,update
 *              delimiter again *
 * @author wuyan
 * @Date 2019.4.11
 * @version 1.00
 */
public class UpdateDelimiter18082 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "目录aa/bb/object18082";
    private String delimiter1 = "%";
    private String delimiter2 = "//";
    private String bucketName = "bucket18082";
    private AmazonS3 s3Client = null;
    private AtomicInteger count = new AtomicInteger();
    private int keyNum = 1000;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test(invocationCount = 1000, threadPoolSize = 50)
    private void testCreateObject() throws Exception {
        AmazonS3 s3Client1 = CommLib.buildS3Client();
        try {
            int num = count.getAndIncrement();
            String subKeyName = keyName + "_" + num + "%aa%test.png";
            s3Client1.putObject( bucketName, subKeyName, "testContent_18082" );
        } finally {
            if ( s3Client1 != null ) {
                s3Client1.shutdown();
            }
        }

    }

    @Test(dependsOnMethods = "testCreateObject")
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        UpdateDelimiter updateDelimiterFirst = new UpdateDelimiter(
                delimiter1 );
        UpdateDelimiter updateDelimiterSecond = new UpdateDelimiter(
                delimiter2 );
        threadExec.addWorker( updateDelimiterFirst );
        threadExec.addWorker( updateDelimiterSecond );
        threadExec.run();

        checkResult( updateDelimiterFirst, updateDelimiterSecond );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                for ( int i = 0; i < keyNum; i++ ) {
                    String subKeyName = keyName + "_" + i + "%aa%test.png";
                    s3Client.deleteObject( bucketName, subKeyName );
                }
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkResult( UpdateDelimiter updateDelimiterFirst,
            UpdateDelimiter updateDelimiterSecond ) throws Exception {
        int errorCodeFirst = updateDelimiterFirst.getRetCode();
        int errorCodeSecond = updateDelimiterSecond.getRetCode();

        // the thread exec success,return errorCode is 0
        boolean isFirstUpdateOk = false;
        boolean isSecondUpdateOk = false;
        if ( errorCodeFirst == 0 ) {
            isFirstUpdateOk = true;
        }
        if ( errorCodeSecond == 0 ) {
            isSecondUpdateOk = true;
        }

        // only one update success,409:DelimiterNotStable,
        // 0:updateDelimierSuccess
        if ( isFirstUpdateOk || !isSecondUpdateOk ) {
            DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter1 );
            Assert.assertEquals( errorCodeSecond, 409,
                    "update delimiter fail! e=" + errorCodeSecond );
            Assert.assertEquals( errorCodeFirst, 0,
                    "update delimiter should be success! e=" + errorCodeFirst );

        } else if ( !isFirstUpdateOk || isSecondUpdateOk ) {
            DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter2 );
            Assert.assertEquals( errorCodeFirst, 409,
                    "update delimiter fail! e=" + errorCodeFirst );
            Assert.assertEquals( errorCodeSecond, 0,
                    "update delimiter should be success! e="
                            + errorCodeSecond );
        } else {
            Assert.fail( "Unexpected result! isFirstUpdateOk=" + isFirstUpdateOk
                    + " and errCode=" + errorCodeFirst + "\n isSecondUpdateOk="
                    + isSecondUpdateOk + " and errCode=" + errorCodeSecond );

        }
    }

    private class UpdateDelimiter extends ResultStore {
        private String delimiter;

        private UpdateDelimiter( String delimiter ) {
            this.delimiter = delimiter;
        }

        @ExecuteOrder(step = 1)
        private void updateDelimiter() {
            try {
                DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
            } catch ( AmazonS3Exception e ) {
                int errCode = e.getStatusCode();
                saveResult( errCode, e );
            }
        }
    }

}
