package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.AmazonS3Exception;
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

/**
 * @Description seqDB-18721: repeat abort multipart upload after complete
 *              multipart upload.
 * @author wuyan
 * @Date 2019.07.31
 * @version 1.00
 */
public class AbortMultipartUpload18721 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/aa/object18721";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private int fileSize = 1024 * 1024 * 15;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void abortMultipartUpload() throws Exception {
        File file = new File( filePath );
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, S3TestBase.bucketName, keyName );
        PartUploadUtils
                .partUpload( s3Client, S3TestBase.bucketName, keyName, uploadId,
                        file );
        AbortMultipartUploadRequest request = new AbortMultipartUploadRequest(
                S3TestBase.bucketName, keyName, uploadId );
        s3Client.abortMultipartUpload( request );

        // repeat abort multipart upload
        try {
            AbortMultipartUploadRequest request1 = new AbortMultipartUploadRequest(
                    S3TestBase.bucketName, keyName, uploadId );
            s3Client.abortMultipartUpload( request1 );
            Assert.fail( "AbortMultipartUpload must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload",
                    "---statuscode=" + e.getStatusCode() );
        }

        // check upload result
        PartUploadUtils.checkAbortMultipartUploadResult( s3Client,
                S3TestBase.bucketName, keyName, uploadId );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
