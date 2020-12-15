package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-19321:复制对象，源对象不存在 ; seqDB-19326:指定桶不存在
 * @author wuyan
 * @Date 2019.09.18
 * @version 1.00
 */
public class CopyObject19321_19326 extends S3TestBase {
    private boolean runSuccess = false;
    private String srcKeyName = "/src/object19321";
    private String destKeyName = "/dest/object19321";
    private String bucketName = "bucket19321";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, srcKeyName, "test copy object." );
    }

    @Test
    public void testCopyObject() throws Exception {
        // test 19321 a: copy object with versionId
        try {
            String versionId = "1";
            CopyObjectRequest request = new CopyObjectRequest( bucketName,
                    srcKeyName, versionId, bucketName, destKeyName );
            s3Client.copyObject( request );
            Assert.fail( "copyObject must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchVersion",
                    e.getStatusCode() + e.getErrorMessage() );
        }

        // test 19321 b: copy object with no versionId
        try {
            String keyName = "test19321";
            s3Client.copyObject( bucketName, keyName, bucketName, destKeyName );
            Assert.fail( "copyObject must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey",
                    e.getStatusCode() + e.getErrorMessage() );
        }

        // testcase 19326: copy object with no srcBucket
        try {
            String srcBucketName = "test19326";
            s3Client.copyObject( srcBucketName, srcKeyName, bucketName,
                    destKeyName );
            Assert.fail( "copyObject must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket",
                    e.getStatusCode() + e.getErrorMessage() );
        }

        // testcase 19326: copy object with no destBucket
        try {
            String destBucketName = "test19326";
            s3Client.copyObject( bucketName, srcKeyName, destBucketName,
                    destKeyName );
            Assert.fail( "copyObject must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket",
                    e.getStatusCode() + e.getErrorMessage() );
        }

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

}
