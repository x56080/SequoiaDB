package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
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
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18679:上传多个分段，其中后面部分分段长度不一致
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class UploadPart18679 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private AmazonS3 s3Client;
    private File localPath;
    private String key = "/aa/bb/obj18679";

    @DataProvider(name = "partSizeProvider")
    private Object[][] generateFileSize() {
        // parameter : partSize1, partSize2, ......
        return new Object[][] {
                // test point a: partSize irregular order
                new Object[] { 5 * 1024 * 1024, 5 * 1024 * 1024,
                        15 * 1024 * 1024, 7 * 1024 * 1024, 11 * 1024 * 1024,
                        6 * 1024 * 1024 },
                // test point b: partSize ascending order
                new Object[] { 5 * 1024 * 1024, 5 * 1024 * 1024,
                        6 * 1024 * 1024, 8 * 1024 * 1024, 11 * 1024 * 1024,
                        17 * 1024 * 1024 },
                // test point c: partSize is descending order
                new Object[] { 15 * 1024 * 1024, 14 * 1024 * 1024,
                        10 * 1024 * 1024, 8 * 1024 * 1024, 6 * 1024 * 1024,
                        5 * 1024 * 1024 } };
    }

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        s3Client = CommLib.buildS3Client();
    }

    @Test(dataProvider = "partSizeProvider")
    private void test( long partSize1, long partSize2, long partSize3,
            long partSize4, long partSize5, long partSize6 ) throws Exception {
        // init file
        long fileSize = partSize1 + partSize2 + partSize3 + partSize4
                + partSize5 + partSize6;
        String filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );

        // upload part
        String uploadId = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, key );
        long[] partSizes = { partSize1, partSize2, partSize3, partSize4,
                partSize5, partSize6 };
        List< PartETag > partETags = this.partUpload( uploadId, partSizes,
                filePath );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, key,
                uploadId, partETags );

        // check results
        File downloadPath = new File(
                localPath + File.separator + "downloadFile_" + fileSize );
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, downloadPath,
                bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );

        actSuccessTests.getAndIncrement();
        // remove the file when the param success
        TestTools.LocalFile.removeFile( filePath );
        TestTools.LocalFile.removeFile( downloadPath );
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests
                    .getAndIncrement() == generateFileSize().length ) {
                s3Client.deleteObject( S3TestBase.bucketName, key );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private List< PartETag > partUpload( String uploadId, long[] partSizes,
            String filePath ) {
        File file = new File( filePath );
        int fileOffset = 0;
        List< PartETag > partEtags = new ArrayList<>();
        for ( int i = 0; i < partSizes.length; i++ ) {
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( fileOffset )
                    .withPartNumber( i + 1 ).withPartSize( partSizes[ i ] )
                    .withBucketName( bucketName ).withKey( key )
                    .withUploadId( uploadId );
            UploadPartResult partResult = s3Client.uploadPart( partRequest );

            partEtags.add( partResult.getPartETag() );
            fileOffset += partSizes[ i ];
        }
        return partEtags;
    }
}