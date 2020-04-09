package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
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
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 完成分段上传，其中指定key和uploadId不一致 testlink-case: seqDB-18715
 *
 * @author wangkexin
 * @Date 2019.8.1
 * @version 1.00
 */
public class UploadPart18715 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18715";
    private String keyNameA = "key18715a";
    private String keyNameB = "key18715b";
    private AmazonS3 s3Client = null;
    private long fileSize = 15 * 1024 * 1024;
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
        // 对象A正在上传
        String uploadIdA = uploadObjectA();

        // 初始化对象B
        String uploadIdB = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyNameB );
        List< PartETag > partEtagsB = PartUploadUtils.partUpload( s3Client,
                bucketName, keyNameB, uploadIdB, file );
        String wrongUploadId = "upload18715";
        // 完成分段上传指定uploadId不存在
        try {
            PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                    keyNameB, wrongUploadId, partEtagsB );
            Assert.fail(
                    "complete multipart upload with non-existent uploadId should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload" );
        }

        // 完成分段上传指定uploadId为对象的uploadId
        try {
            PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                    keyNameB, uploadIdA, partEtagsB );
            Assert.fail(
                    "complete multipart upload with uploadId of other keys should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload" );
        }

        // 对象B完成分段上传
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyNameB,
                uploadIdB, partEtagsB );

        // check
        String expMd5 = TestTools.getMD5( filePath );
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyNameB );
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

    private String uploadObjectA() {
        List< PartETag > partEtags = new ArrayList<>();
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyNameA );
        long partSize = PartUploadUtils.partLimitMinSize;
        UploadPartRequest partRequest = new UploadPartRequest().withFile( file )
                .withFileOffset( 0 ).withPartNumber( 1 )
                .withPartSize( partSize ).withBucketName( bucketName )
                .withKey( keyNameA ).withUploadId( uploadId );
        UploadPartResult uploadPartResult = s3Client.uploadPart( partRequest );
        partEtags.add( uploadPartResult.getPartETag() );
        return uploadId;
    }
}
