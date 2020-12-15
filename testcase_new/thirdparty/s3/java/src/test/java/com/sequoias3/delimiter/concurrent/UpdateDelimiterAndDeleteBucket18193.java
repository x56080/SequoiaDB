package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
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

/**
 * @Description seqDB-18190: concurrent update delimiter and remove bucket
 * @author wuyan
 * @Date 2019.5.8
 * @version 1.00
 */
public class UpdateDelimiterAndDeleteBucket18193 extends S3TestBase {
    private boolean runSuccess = false;
    private String delimiter = "%";
    private String bucketName = "bucket18193";
    private String keyName = "/aa18193%object18193";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        s3Client.putObject( bucketName, keyName, keyName + "testconent" );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        UpdateDelimiter updateDelimiter = new UpdateDelimiter();
        DeleteBucket deleteBucket = new DeleteBucket();

        threadExec.addWorker( updateDelimiter );
        threadExec.addWorker( deleteBucket );
        threadExec.run();

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private class UpdateDelimiter extends ResultStore {
        @ExecuteOrder(step = 1)
        private void updateDelimiter() {
            try {
                DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
            } catch ( AmazonS3Exception e ) {
                int errCode = e.getStatusCode();
                // 404:NoSuchBucket
                Assert.assertEquals( errCode, 404,
                        "errCode:" + e.getErrorCode() + "," + e.getMessage() );
            }
        }
    }

    private class DeleteBucket {
        @ExecuteOrder(step = 1)
        private void deleteBucket() {
            s3Client.deleteVersion( bucketName, keyName, "0" );
            s3Client.deleteBucket( bucketName );
        }

        @ExecuteOrder(step = 2)
        private void checkResult() {
            @SuppressWarnings("deprecation")
            boolean isExist = s3Client.doesBucketExist( bucketName );
            Assert.assertFalse( isExist );
        }
    }

}
