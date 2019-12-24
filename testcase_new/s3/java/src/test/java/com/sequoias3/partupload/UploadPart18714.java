package com.sequoias3.partupload;

import com.amazonaws.ClientConfiguration;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.AWSStaticCredentialsProvider;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.client.builder.AwsClientBuilder;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.AmazonS3ClientBuilder;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 上传分段，其中指定key和uploadId不一致 testlink-case: seqDB-18714
 *
 * @author wangkexin
 * @Date 2019.8.1
 * @version 1.00
 */
public class UploadPart18714 extends S3TestBase {
    private boolean runSuccess = false;
    private String AWS_ACCESS_KEY = "ABCDEFGHIJKLMNOPQRST";
    private String AWS_SECRET_KEY = "abcdefghijklmnopqrstuvwxyz0123456789ABCD";
    private String clientRegion = "us-east-1";
    private String bucketName = "bucket18714";
    private String keyNameA = "key18714a";
    private String keyNameB = "key18714b";
    private AmazonS3 s3Client = null;
    private long fileSize = 10 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        s3Client.shutdown();
    }

    @Test
    private void test() throws Exception {
        testExpectContinue( true );
        testExpectContinue( false );
        runSuccess = true;
    }

    private void testExpectContinue( boolean useExpectContinue )
            throws Exception {
        AmazonS3 s3Client = null;
        try {
            if ( useExpectContinue ) {
                s3Client = buildS3ClientUseExpectContinue();
            } else {
                s3Client = CommLib.buildS3Client();
            }
            testUpload( s3Client );
        } finally {
            s3Client.shutdown();
        }
    }

    private void testUpload( AmazonS3 s3 ) throws Exception {
        // 对象A正在上传
        List<PartETag> partEtags = new ArrayList<>();
        String uploadIdA = uploadObjectA( s3, partEtags );

        // 初始化对象B
        PartUploadUtils.initPartUpload( s3, bucketName, keyNameB );
        String wrongUploadId = "18714";
        // 上传分段指定uploadId不存在
        try {
            PartUploadUtils.partUpload( s3, bucketName, keyNameB, wrongUploadId,
                    file );
            Assert.fail(
                    "upload part with non-existent uploadId should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload" );
        }

        // 上传分段指定uploadId为对象A的uploadId
        try {
            PartUploadUtils
                    .partUpload( s3, bucketName, keyNameB, uploadIdA, file );
            Assert.fail(
                    "upload part with uploadId of other keys should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload" );
        }

        // 对象A完成分段上传
        PartUploadUtils
                .completeMultipartUpload( s3, bucketName, keyNameA, uploadIdA,
                        partEtags );
        Assert.assertFalse( s3.doesObjectExist( bucketName, keyNameB ) );
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client = CommLib.buildS3Client();
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private String uploadObjectA( AmazonS3 s3, List<PartETag> partEtags ) {
        String uploadId = PartUploadUtils
                .initPartUpload( s3, bucketName, keyNameA );
        long partSize = 5 * 1024;
        UploadPartRequest partRequest = new UploadPartRequest().withFile( file )
                .withFileOffset( 0 ).withPartNumber( 1 )
                .withPartSize( partSize ).withBucketName( bucketName )
                .withKey( keyNameA ).withUploadId( uploadId );
        UploadPartResult uploadPartResult = s3.uploadPart( partRequest );
        partEtags.add( uploadPartResult.getPartETag() );
        return uploadId;
    }

    private AmazonS3 buildS3ClientUseExpectContinue() {
        AmazonS3 s3Client = null;
        AWSCredentials credentials = new BasicAWSCredentials( AWS_ACCESS_KEY,
                AWS_SECRET_KEY );
        AwsClientBuilder.EndpointConfiguration endpointConfiguration = new AwsClientBuilder.EndpointConfiguration(
                S3TestBase.s3ClientUrl, clientRegion );
        ClientConfiguration config = new ClientConfiguration();
        config.setUseExpectContinue( true );
        config.setSocketTimeout( 300000 );
        s3Client = AmazonS3ClientBuilder.standard()
                .withEndpointConfiguration( endpointConfiguration )
                .withClientConfiguration( config )
                .withChunkedEncodingDisabled( true )
                .withPathStyleAccessEnabled( true ).withCredentials(
                        new AWSStaticCredentialsProvider( credentials ) )
                .build();
        return s3Client;
    }
}
