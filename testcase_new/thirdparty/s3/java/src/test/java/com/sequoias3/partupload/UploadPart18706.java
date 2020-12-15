package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description: 开启分段检测，指定分段长度不正确 testlink-case: seqDB-18706
 *
 * @author wangkexin
 * @Date 2019.7.30
 * @version 1.00
 */
public class UploadPart18706 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String bucketName = "bucket18706";
    private String keyName = "key18706";
    private AmazonS3 s3Client = null;
    private long fileSize = 15 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;

    @DataProvider(name = "partSizeProvider")
    public Object[][] generateObjectNumber() {
        return new Object[][] {
                // test a : 5M-1
                new Object[] { 5 * 1024 * 1024 - 1 },
                // test b : 4M
                new Object[] { 4 * 1024 * 1024 }, };
    }

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

    @Test(dataProvider = "partSizeProvider")
    private void testUpload( long partSize ) throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        List< PartETag > partEtags = PartUploadUtils.partUpload( s3Client,
                bucketName, keyName, uploadId, file, partSize );
        try {
            PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                    keyName, uploadId, partEtags );
            Assert.fail( "complete multipart upload should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "EntityTooSmall" );
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generateObjectNumber().length ) {
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
