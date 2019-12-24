package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-19309:指定源对象为删除标记对象
 * @author wuyan
 * @Date 2019.09.17
 * @version 1.00
 */
public class CopyObject19309 extends S3TestBase {
    private boolean runSuccess = false;
    private String srcKeyName = "/object19309a";
    private String destKeyName = "/object19309b";
    private String srcBucketName = "bucket19309a";
    private String destBucketName = "bucket19309b";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, srcBucketName );
        CommLib.clearBucket( s3Client, destBucketName );

        s3Client.createBucket( srcBucketName );
        s3Client.createBucket( destBucketName );
        CommLib.setBucketVersioning( s3Client, srcBucketName, "Enabled" );
        // put a deleteTag object
        s3Client.deleteObject( srcBucketName, srcKeyName );
    }

    @Test
    public void testCopyObject() throws Exception {
        // test a: the srcBucket is different from the destBucket
        copyDeleteTagObjectAndCheckResult( srcBucketName, srcKeyName,
                destBucketName, destKeyName );

        // test b: the srcBucket is the same as destBucket
        copyDeleteTagObjectAndCheckResult( srcBucketName, srcKeyName,
                srcBucketName, destKeyName );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, srcBucketName );
                CommLib.clearBucket( s3Client, destBucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void copyDeleteTagObjectAndCheckResult( String srcBucketName,
            String srcKeyName, String destBucketName, String destKeyName ) {
        try {
            s3Client.copyObject( srcBucketName, srcKeyName, destBucketName,
                    destKeyName );
            Assert.fail( "copyObject must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey",
                    e.getStatusCode() + e.getErrorMessage() );
        }

        boolean isExist = s3Client
                .doesObjectExist( destBucketName, destKeyName );
        Assert.assertFalse( isExist );
    }
}
