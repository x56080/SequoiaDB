package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * test content: AbortMultipartUploadRequest接口参数校验 testlink-case: seqDB-18812
 *
 * @author wangkexin
 * @Date 2019.8.7
 * @version 1.00
 */
public class AbortMultipartUploadRequest18812 extends S3TestBase {
    private String bucketName = "bucket18812";
    private String keyName = "key18812";
    private AmazonS3 s3Client = null;
    private long fileSize = 15 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    public void testIllegalParameter() throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        PartUploadUtils.partUpload( s3Client, bucketName, keyName, uploadId,
                file );

        // a.接口参数取值合法---已在功能测试中验证
        // b.接口参数取值非法---对象名为空串""，null
        AbortMultipartUploadRequest request = new AbortMultipartUploadRequest(
                bucketName, "", uploadId );
        try {
            s3Client.abortMultipartUpload( request );
            Assert.fail( "when keyName is '', it should fail." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "InvalidRequest" ) || !e
                    .getErrorMessage().equals( "A key must be specified." ) ) {
                throw e;
            }
        }

        request = new AbortMultipartUploadRequest( bucketName, null, uploadId );
        try {
            s3Client.abortMultipartUpload( request );
            Assert.fail( "when keyName is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The key parameter must be specified when aborting a multipart upload" );
        }

        // 桶名为null
        request = new AbortMultipartUploadRequest( null, keyName, uploadId );
        try {
            s3Client.abortMultipartUpload( request );
            Assert.fail( "when bucketName is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The bucket name parameter must be specified when aborting a multipart upload" );
        }

        // uploadId为null
        request = new AbortMultipartUploadRequest( bucketName, keyName, null );
        try {
            s3Client.abortMultipartUpload( request );
            Assert.fail( "when uploadId is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The upload ID parameter must be specified when aborting a multipart upload" );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
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