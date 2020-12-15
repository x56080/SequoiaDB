package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.InitiateMultipartUploadRequest;
import com.amazonaws.services.s3.model.InitiateMultipartUploadResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description:InitiateMultipartUploadRequest接口参数校验 testlink-case: seqDB-18806
 *
 * @author wangkexin
 * @Date 2019.8.6
 * @version 1.00
 */
public class InitiateMultipartUploadRequest18806 extends S3TestBase {
    private String bucketName = "bucket18806";
    private AmazonS3 s3Client = null;
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );

    @DataProvider(name = "legalKeyNameProvider")
    public Object[][] generateKeyName() {
        return new Object[][] {
                // test a : 范围内取值
                new Object[] { "/dir1/test.txt" },
                // test b : 长度边界值
                new Object[] { ObjectUtils.getRandomString( 1 ) },
                new Object[] { ObjectUtils.getRandomString( 900 ) },
                // test c : 包含特殊字符
                new Object[] { "!-_.*'()" },
                // test d : 包含 数字字符[0-9a-zA-Z]
                new Object[] {
                        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" },
                // test e : 包含需要特殊处理的字符
                new Object[] { "&@:,$=+?;" + " " },
                // test f : 包含不建议使用的字符
                new Object[] { "\\^`><{}][#%“~|" },
                // test g : 包含中文字符
                new Object[] { "测试对象名" }, };
    }

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test(dataProvider = "legalKeyNameProvider")
    public void testLegalKeyName( String keyName ) throws Exception {
        InitiateMultipartUploadRequest initRequest = new InitiateMultipartUploadRequest(
                bucketName, keyName );
        InitiateMultipartUploadResult result = s3Client
                .initiateMultipartUpload( initRequest );
        String uploadId = result.getUploadId();
        Assert.assertNotEquals( uploadId, null );
        actSuccessTests.getAndIncrement();
    }

    @Test
    public void testIllegalKeyName() throws Exception {
        // test a : 对象名为空串，null，901个字节
        InitiateMultipartUploadRequest initRequest = new InitiateMultipartUploadRequest(
                bucketName, "" );
        try {
            s3Client.initiateMultipartUpload( initRequest );
            Assert.fail( "when key name is '',it should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidRequest" );
        }

        initRequest = new InitiateMultipartUploadRequest( bucketName, null );
        try {
            s3Client.initiateMultipartUpload( initRequest );
            Assert.fail( "when key name is null,it should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The key parameter must be specified when initiating a multipart upload" );
        }

        initRequest = new InitiateMultipartUploadRequest( bucketName,
                ObjectUtils.getRandomString( 901 ) );
        try {
            s3Client.initiateMultipartUpload( initRequest );
            Assert.fail( "when key name is 901 characters,it should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorMessage(), "Your key is too long." );
        }

        // test b : 桶名为null
        initRequest = new InitiateMultipartUploadRequest( null,
                "/dir/test18806" );
        try {
            s3Client.initiateMultipartUpload( initRequest );
            Assert.fail( "when bucket name is null,it should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The bucket name parameter must be specified when initiating a multipart upload" );
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == ( generateKeyName().length + 1 ) ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
