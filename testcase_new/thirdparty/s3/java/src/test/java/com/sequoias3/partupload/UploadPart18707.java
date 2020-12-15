package com.sequoias3.partupload;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
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
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.List;

/**
 * @Description: 相同key相同uploadId多次分段上传 testlink-case: seqDB-18707
 *
 * @author wangkexin
 * @Date 2019.7.30
 * @version 1.00
 */
public class UploadPart18707 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18707";
    private String keyName = "key18707";
    private AmazonS3 s3Client = null;
    private long fileSize = 10 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
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
    private void testUpload() throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        List< PartETag > partEtags = PartUploadUtils.partUpload( s3Client,
                bucketName, keyName, uploadId, file );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );

        // 再次指定相同key和相同uploadId，执行分段上传
        try {
            partEtags = PartUploadUtils.partUpload( s3Client, bucketName,
                    keyName, uploadId, file );
            Assert.fail( "upload part again should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload" );
        } catch ( SdkClientException e ) {
            Assert.assertEquals( e.getMessage(),
                    "Unable to execute HTTP request: Connection reset by peer: socket write error" );
        }

        try {
            PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                    keyName, uploadId, partEtags );
            Assert.fail( "complete multipart upload again should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload" );
        }

        String expMd5 = TestTools.getMD5( filePath );
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
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
            s3Client.shutdown();
        }
    }
}
