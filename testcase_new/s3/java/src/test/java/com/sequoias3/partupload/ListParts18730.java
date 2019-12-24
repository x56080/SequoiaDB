package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.ListPartsRequest;
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
 * @Description seqDB-18730: list parts.specified uploadId or keyName are not
 *              consistent
 * @author wuyan
 * @Date 2019.08.01
 * @version 1.00
 */
public class ListParts18730 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/aa/object18730";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private int fileSize = 1024 * 1024 * 10;

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
    public void listParts() throws Exception {
        File file = new File( filePath );
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, S3TestBase.bucketName, keyName );
        PartUploadUtils
                .partUpload( s3Client, S3TestBase.bucketName, keyName, uploadId,
                        file );

        // test a: the uploadId is not exist
        try {
            String uploadIdA = "18730";
            ListPartsRequest listRequest = new ListPartsRequest( bucketName,
                    keyName, uploadIdA );
            s3Client.listParts( listRequest );
            Assert.fail( "listParts must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload",
                    "---statuscode=" + e.getStatusCode() );
        }

        // test b: specified uploadId is belong to object:"/aa/object18730"
        try {
            String keyNameB = "object18730B";
            ListPartsRequest listRequest = new ListPartsRequest( bucketName,
                    keyNameB, uploadId );
            s3Client.listParts( listRequest );
            Assert.fail( "listParts must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload",
                    "---statuscode=" + e.getStatusCode() );
        }

        AbortMultipartUploadRequest request = new AbortMultipartUploadRequest(
                S3TestBase.bucketName, keyName, uploadId );
        s3Client.abortMultipartUpload( request );
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
