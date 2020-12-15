package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-18718: abort multipart upload after upload multiple parts
 * @author wuyan
 * @Date 2019.07.30
 * @version 1.00
 */
public class AbortMultipartUpload18718 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/aa/maa/bb/object18718";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private int fileSize = 1024 * 1024 * 27;

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
        // test a: upload parts is different length
        String uploadIdA = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, keyName );
        int[] partSizes = { 1024 * 1024 * 6, 1024 * 1024 * 5, 1024 * 1024 * 6,
                1024 * 1024 * 10 };
        partUpload( uploadIdA, file, partSizes );
        AbortMultipartUploadRequest requestA = new AbortMultipartUploadRequest(
                S3TestBase.bucketName, keyName, uploadIdA );
        s3Client.abortMultipartUpload( requestA );
        PartUploadUtils.checkAbortMultipartUploadResult( s3Client,
                S3TestBase.bucketName, keyName, uploadIdA );

        // test b: upload parts is the same length
        String uploadIdB = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, keyName );
        PartUploadUtils.partUpload( s3Client, S3TestBase.bucketName, keyName,
                uploadIdB, file );
        AbortMultipartUploadRequest requestB = new AbortMultipartUploadRequest(
                S3TestBase.bucketName, keyName, uploadIdB );
        s3Client.abortMultipartUpload( requestB );
        PartUploadUtils.checkAbortMultipartUploadResult( s3Client,
                S3TestBase.bucketName, keyName, uploadIdB );

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

    private void partUpload( String uploadId, File file, int[] partSizes ) {
        int filePosition = 0;
        for ( int i = 0; i < partSizes.length; i++ ) {
            int partNumber = i + 1;
            long eachPartSize = partSizes[ i ];
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( partNumber ).withPartSize( eachPartSize )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            s3Client.uploadPart( partRequest );
            filePosition += eachPartSize;
        }

    }
}
