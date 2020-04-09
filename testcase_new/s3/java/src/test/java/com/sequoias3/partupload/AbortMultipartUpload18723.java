package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
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
 * @Description seqDB-18723: upload the same key with different uploadId,
 *              reUpload parts after abort multipart upload.
 * @author wuyan
 * @Date 2019.07.31
 * @version 1.00
 */
public class AbortMultipartUpload18723 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/aa/object18723";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;
    private String filePath3 = null;
    private int fileSize1 = 1024 * 1024 * 15;
    private int fileSize2 = 1024 * 1024 * 10;
    private int fileSize3 = 1024 * 1024 * 20;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath1 = localPath + File.separator + "localFile_" + fileSize1
                + ".txt";
        filePath2 = localPath + File.separator + "localFile_" + fileSize2
                + ".txt";
        filePath3 = localPath + File.separator + "localFile_" + fileSize3
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath1, fileSize1 );
        TestTools.LocalFile.createFile( filePath2, fileSize2 );
        TestTools.LocalFile.createFile( filePath3, fileSize3 );
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void abortMultipartUpload() throws Exception {
        File file1 = new File( filePath1 );
        File file2 = new File( filePath2 );
        File file3 = new File( filePath3 );
        String uploadId1 = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, keyName );
        PartUploadUtils.partUpload( s3Client, S3TestBase.bucketName, keyName,
                uploadId1, file1 );
        String uploadId2 = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, keyName );
        PartUploadUtils.partUpload( s3Client, S3TestBase.bucketName, keyName,
                uploadId2, file2 );
        String uploadId3 = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, keyName );
        List< PartETag > partEtags = PartUploadUtils.partUpload( s3Client,
                S3TestBase.bucketName, keyName, uploadId3, file3 );

        AbortMultipartUploadRequest request1 = new AbortMultipartUploadRequest(
                S3TestBase.bucketName, keyName, uploadId1 );
        s3Client.abortMultipartUpload( request1 );
        PartUploadUtils.checkAbortMultipartUploadResult( s3Client,
                S3TestBase.bucketName, keyName, uploadId1 );

        AbortMultipartUploadRequest request2 = new AbortMultipartUploadRequest(
                S3TestBase.bucketName, keyName, uploadId2 );
        s3Client.abortMultipartUpload( request2 );
        PartUploadUtils.checkAbortMultipartUploadResult( s3Client,
                S3TestBase.bucketName, keyName, uploadId2 );

        PartUploadUtils.completeMultipartUpload( s3Client,
                S3TestBase.bucketName, keyName, uploadId3, partEtags );

        // down file check the file content
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                S3TestBase.bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath3 ) );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( S3TestBase.bucketName, keyName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
