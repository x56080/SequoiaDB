package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUploadListing;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18738: lists in-progress multipart uploads by bucket.
 * @author wuyan
 * @Date 2019.07.31
 * @version 1.00
 */
public class ListMultipartUploads18738 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18738";
    private String keyNameA = "/aa/object18738A";
    private String keyNameB = "/aa/object18738B";
    private String keyNameC = "/aa/object18738C";
    private String keyNameD = "/aa/object18738D";
    private String keyNameE = "/aa/object18738E";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private File file = null;
    private int fileSize = 1024 * 1024 * 20;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void listMultipartUploads() throws Exception {
        // test a: PartUpload
        String uploadIdA = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameA );
        PartUploadUtils
                .partUpload( s3Client, bucketName, keyNameA, uploadIdA, file );

        // test b: initPartUpload
        String uploadIdB = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameB );

        // test c: upload partial uploads
        String uploadIdC = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameC );
        UploadPartRequest partRequest = new UploadPartRequest().withFile( file )
                .withFileOffset( 0 ).withPartNumber( 1 )
                .withPartSize( 1024 * 1024 * 5 ).withBucketName( bucketName )
                .withKey( keyNameC ).withUploadId( uploadIdC );
        s3Client.uploadPart( partRequest );

        // test d: completeMultipartUpload
        String uploadIdD = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameD );
        List<PartETag> partEtagsD = PartUploadUtils
                .partUpload( s3Client, bucketName, keyNameD, uploadIdD, file );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyNameD,
                uploadIdD, partEtagsD );

        // test e: abortMultipartUpload
        String uploadIdE = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameE );
        AbortMultipartUploadRequest abortRequest = new AbortMultipartUploadRequest(
                bucketName, keyNameE, uploadIdE );
        s3Client.abortMultipartUpload( abortRequest );

        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        MultiValueMap<String, String> expUpload = new LinkedMultiValueMap<String, String>();
        expUpload.add( keyNameA, uploadIdA );
        expUpload.add( keyNameB, uploadIdB );
        expUpload.add( keyNameC, uploadIdC );
        List<String> expCommonPrefixes = new ArrayList<>();
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUpload );
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
