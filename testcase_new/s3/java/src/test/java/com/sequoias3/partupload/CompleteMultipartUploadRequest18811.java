package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CompleteMultipartUploadRequest;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content: CompleteMultipartUploadRequest接口参数校验 testlink-case: seqDB-18811
 *
 * @author wangkexin
 * @Date 2019.8.7
 * @version 1.00
 */
public class CompleteMultipartUploadRequest18811 extends S3TestBase {
    private String bucketName = "bucket18811";
    private String keyName = "key18811";
    private AmazonS3 s3Client = null;
    private long fileSize = 15 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );

    @DataProvider(name = "legalKeyNameProvider")
    public Object[][] generateKeyName() {
        return new Object[][] {
                new Object[] { ObjectUtils.getRandomString( 1 ) },
                new Object[] { ObjectUtils.getRandomString( 900 ) } };
    }

    @BeforeClass
    private void setUp() throws Exception {
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
    }

    @Test(dataProvider = "legalKeyNameProvider")
    public void testLegalKeyName( String keyName ) throws Exception {
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );
        List<PartETag> partEtags = PartUploadUtils
                .partUpload( s3Client, bucketName, keyName, uploadId, file );

        // a.接口参数取值合法---已在功能测试中验证（此处覆盖对象名长度为1和900字节）
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );
        String expMd5 = TestTools.getMD5( filePath );
        String downloadMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
        actSuccessTests.getAndIncrement();
    }

    @Test
    public void testIllegalParameter() throws Exception {
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );
        List<PartETag> partEtags = PartUploadUtils
                .partUpload( s3Client, bucketName, keyName, uploadId, file );

        // b.接口参数取值非法---对象名为空串""，null，901个字节
        CompleteMultipartUploadRequest request = new CompleteMultipartUploadRequest(
                bucketName, "", uploadId, partEtags );
        try {
            s3Client.completeMultipartUpload( request );
            Assert.fail( "when keyName is '', it should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidRequest" );
        }

        request = new CompleteMultipartUploadRequest( bucketName, null,
                uploadId, partEtags );
        try {
            s3Client.completeMultipartUpload( request );
            Assert.fail( "when keyName is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The key parameter must be specified when completing a multipart upload" );
        }

        request = new CompleteMultipartUploadRequest( bucketName,
                ObjectUtils.getRandomString( 901 ), uploadId, partEtags );
        try {
            s3Client.completeMultipartUpload( request );
            Assert.fail( "when key name is 901 characters,it should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload" );
        }

        // 桶名为null
        request = new CompleteMultipartUploadRequest( null, keyName, uploadId,
                partEtags );
        try {
            s3Client.completeMultipartUpload( request );
            Assert.fail( "when bucketName is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The bucket name parameter must be specified when completing a multipart upload" );
        }

        // uploadId为null
        request = new CompleteMultipartUploadRequest( bucketName, keyName, null,
                partEtags );
        try {
            s3Client.completeMultipartUpload( request );
            Assert.fail( "when uploadId is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The upload ID parameter must be specified when completing a multipart upload" );
        }

        // partEtags为null
        request = new CompleteMultipartUploadRequest( bucketName, keyName,
                uploadId, null );
        try {
            s3Client.completeMultipartUpload( request );
            Assert.fail( "when partEtags is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The part ETags parameter must be specified when completing a multipart upload" );
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == ( generateKeyName().length + 1 ) ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}