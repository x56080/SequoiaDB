package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.AmazonS3Exception;
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
 * @Description seqDB-18720: abort multipart upload after complete multipart
 *              upload.
 * @author wuyan
 * @Date 2019.07.31
 * @version 1.00
 */
public class AbortMultipartUpload18720 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/aa/object18720";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private int fileSize = 1024 * 1024 * 38;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void abortMultipartUpload() throws Exception {
        File file = new File( filePath );
        String uploadId = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, keyName );
        int[] partSizes = { 1024 * 1024 * 6, 1024 * 1024 * 5, 1024 * 1024 * 6,
                1024 * 1024 * 8, 1024 * 1024 * 6, 1024 * 1024 * 7 };
        int[] partNumbers = { 2, 4, 6, 10, 1000, 10000 };
        List< PartETag > partEtags = partUpload( uploadId, file, partSizes,
                partNumbers );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );

        // abort multipart upload
        try {
            AbortMultipartUploadRequest request = new AbortMultipartUploadRequest(
                    S3TestBase.bucketName, keyName, uploadId );
            s3Client.abortMultipartUpload( request );
            Assert.fail( "AbortMultipartUpload must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload",
                    "---statuscode=" + e.getStatusCode() );
        }

        // check upload result
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                S3TestBase.bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );

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

    private List< PartETag > partUpload( String uploadId, File file,
            int[] partSizes, int[] partNumbers ) {
        List< PartETag > partEtags = new ArrayList<>();
        int filePosition = 0;
        for ( int i = 0; i < partSizes.length; i++ ) {
            int partNumber = partNumbers[ i ];
            long eachPartSize = partSizes[ i ];
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( partNumber ).withPartSize( eachPartSize )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            partEtags.add( uploadPartResult.getPartETag() );
            filePosition += eachPartSize;
        }
        return partEtags;
    }
}
