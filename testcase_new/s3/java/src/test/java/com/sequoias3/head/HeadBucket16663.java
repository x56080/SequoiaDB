package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.HeadBucketRequest;
import com.amazonaws.services.s3.model.HeadBucketResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.HeadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-16663: test Headbucket
 * @author wuyan
 * @Date 2018.12.12
 * @version 1.00
 */
public class HeadBucket16663 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16663";
    private AmazonS3 s3Client = null;

    @BeforeClass(enabled = false)
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        HeadUtils.clearOneBucket( s3Client, bucketName );
    }

    @Test(enabled = false)
    public void testCreateBucket() {
        s3Client.createBucket(
                new CreateBucketRequest( bucketName, "region16663a" ) );
        HeadBucketRequest request1 = new HeadBucketRequest( bucketName );
        HeadBucketResult result1 = s3Client.headBucket( request1 );
        Assert.assertEquals( result1.getBucketRegion(), "region16663a" );

        s3Client.deleteBucket( bucketName );
        try {
            HeadBucketRequest request2 = new HeadBucketRequest( bucketName );
            s3Client.headBucket( request2 );
            Assert.fail( "head bucket must be fail!" );
        } catch ( AmazonS3Exception e ) {
            // 404 Not Found
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        s3Client.createBucket(
                new CreateBucketRequest( bucketName, "region16663b" ) );
        HeadBucketRequest request3 = new HeadBucketRequest( bucketName );
        HeadBucketResult result3 = s3Client.headBucket( request3 );
        Assert.assertEquals( result3.getBucketRegion(), "region16663b" );

        runSuccess = true;
    }

    @AfterClass(enabled = false)
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

}
