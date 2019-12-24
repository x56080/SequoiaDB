package com.sequoias3.object;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-19587:CopyObjectRequest接口参数校验
 * @author wangkexin
 * @Date 2019.09.27
 * @version 1.00
 */
public class TestCopyObjectRequest19587 extends S3TestBase {
    private String bucketName = "bucket19587";
    private String srcKeyName = "srckey19587";
    private String destKeyName = "destkey19587";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void testIllegalParameter() throws Exception {
        // a.接口参数取值合法---已在功能测试中验证
        // b.接口参数取值非法---sourceBucketName取值为null、空串
        CopyObjectRequest request = new CopyObjectRequest( null, srcKeyName,
                bucketName, destKeyName );
        try {
            s3Client.copyObject( request );
            Assert.fail( "when sourceBucketName is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The source bucket name must be specified when copying an object" );
        }

        request = new CopyObjectRequest( "", srcKeyName, bucketName,
                destKeyName );
        try {
            s3Client.copyObject( request );
            Assert.fail( "when sourceBucketName is '', it should fail." );
        } catch ( AmazonServiceException e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidArgument" );
        }

        // sourceKey取值为null、空串
        request = new CopyObjectRequest( bucketName, null, bucketName,
                destKeyName );
        try {
            s3Client.copyObject( request );
            Assert.fail( "when sourceKey is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The source object key must be specified when copying an object" );
        }

        request = new CopyObjectRequest( bucketName, "", bucketName,
                destKeyName );
        try {
            s3Client.copyObject( request );
            Assert.fail( "when sourceKey is '', it should fail." );
        } catch ( AmazonServiceException e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidArgument" );
        }

        // destinationBucketName取值为null、空串
        request = new CopyObjectRequest( bucketName, srcKeyName, null,
                destKeyName );
        try {
            s3Client.copyObject( request );
            Assert.fail(
                    "when destinationBucketName is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The destination bucket name must be specified when copying an object" );
        }

        request = new CopyObjectRequest( bucketName, srcKeyName, "",
                destKeyName );
        try {
            s3Client.copyObject( request );
            Assert.fail( "when destinationBucketName is '', it should fail." );
        } catch ( AmazonServiceException e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidArgument" );
        }

        // destinationKey取值为null、空串
        request = new CopyObjectRequest( bucketName, srcKeyName, bucketName,
                null );
        try {
            s3Client.copyObject( request );
            Assert.fail( "when destinationKey is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The destination object key must be specified when copying an object" );
        }

        request = new CopyObjectRequest( bucketName, srcKeyName, bucketName,
                "" );
        try {
            s3Client.copyObject( request );
            Assert.fail( "when destinationKey is '', it should fail." );
        } catch ( AmazonServiceException e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidArgument" );
        }
    }

    @AfterClass
    private void tearDown() {
        if ( s3Client != null ) {
            s3Client.shutdown();
        }
    }
}